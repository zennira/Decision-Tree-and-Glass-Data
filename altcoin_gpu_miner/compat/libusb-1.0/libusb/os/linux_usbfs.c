
/* -*- Mode: C; c-basic-offset:8 ; indent-tabs-mode:t -*- */
/*
 * Linux usbfs backend for libusb
 * Copyright (C) 2007-2009 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>
 * Copyright (c) 2013 Nathan Hjelm <hjelmn@mac.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "libusb.h"
#include "libusbi.h"
#include "linux_usbfs.h"

/* sysfs vs usbfs:
 * opening a usbfs node causes the device to be resumed, so we attempt to
 * avoid this during enumeration.
 *
 * sysfs allows us to read the kernel's in-memory copies of device descriptors
 * and so forth, avoiding the need to open the device:
 *  - The binary "descriptors" file was added in 2.6.23.
 *  - The "busnum" file was added in 2.6.22
 *  - The "devnum" file has been present since pre-2.6.18
 *  - the "bConfigurationValue" file has been present since pre-2.6.18
 *
 * If we have bConfigurationValue, busnum, and devnum, then we can determine
 * the active configuration without having to open the usbfs node in RDWR mode.
 * We assume this is the case if we see the busnum file (indicates 2.6.22+).
 * The busnum file is important as that is the only way we can relate sysfs
 * devices to usbfs nodes.
 *
 * If we also have descriptors, we can obtain the device descriptor and active 
 * configuration without touching usbfs at all.
 *
 * The descriptors file originally only contained the active configuration
 * descriptor alongside the device descriptor, but all configurations are
 * included as of Linux 2.6.26.
 */

/* endianness for multi-byte fields:
 *
 * Descriptors exposed by usbfs have the multi-byte fields in the device
 * descriptor as host endian. Multi-byte fields in the other descriptors are
 * bus-endian. The kernel documentation says otherwise, but it is wrong.
 */

const char *usbfs_path = NULL;

/* use usbdev*.* device names in /dev instead of the usbfs bus directories */
static int usbdev_names = 0;

/* Linux 2.6.32 adds support for a bulk continuation URB flag. this basically
 * allows us to mark URBs as being part of a specific logical transfer when
 * we submit them to the kernel. then, on any error except a cancellation, all
 * URBs within that transfer will be cancelled and no more URBs will be
 * accepted for the transfer, meaning that no more data can creep in.
 *
 * The BULK_CONTINUATION flag must be set on all URBs within a bulk transfer
 * (in either direction) except the first.
 * For IN transfers, we must also set SHORT_NOT_OK on all URBs except the
 * last; it means that the kernel should treat a short reply as an error.
 * For OUT transfers, SHORT_NOT_OK must not be set. it isn't needed (OUT
 * transfers can't be short unless there's already some sort of error), and
 * setting this flag is disallowed (a kernel with USB debugging enabled will
 * reject such URBs).
 */
static int supports_flag_bulk_continuation = -1;

/* Linux 2.6.31 fixes support for the zero length packet URB flag. This
 * allows us to mark URBs that should be followed by a zero length data
 * packet, which can be required by device- or class-specific protocols.
 */
static int supports_flag_zero_packet = -1;

/* clock ID for monotonic clock, as not all clock sources are available on all
 * systems. appropriate choice made at initialization time. */
static clockid_t monotonic_clkid = -1;

/* do we have a busnum to relate devices? this also implies that we can read
 * the active configuration through bConfigurationValue */
static int sysfs_can_relate_devices = 0;

/* do we have a descriptors file? */
static int sysfs_has_descriptors = 0;

/* how many times have we initted (and not exited) ? */
static volatile int init_count = 0;

/* lock for init_count */
static pthread_mutex_t hotplug_lock = PTHREAD_MUTEX_INITIALIZER;

static int linux_start_event_monitor(void);
static int linux_stop_event_monitor(void);
static int linux_scan_devices(struct libusb_context *ctx);

#if !defined(USE_UDEV)
static int linux_default_scan_devices (struct libusb_context *ctx);
#endif

struct linux_device_priv {
	char *sysfs_dir;
	unsigned char *dev_descriptor;
	unsigned char *config_descriptor;
};

struct linux_device_handle_priv {
	int fd;
	uint32_t caps;
};

enum reap_action {
	NORMAL = 0,
	/* submission failed after the first URB, so await cancellation/completion
	 * of all the others */
	SUBMIT_FAILED,

	/* cancelled by user or timeout */
	CANCELLED,

	/* completed multi-URB transfer in non-final URB */
	COMPLETED_EARLY,

	/* one or more urbs encountered a low-level error */
	ERROR,
};

struct linux_transfer_priv {
	union {
		struct usbfs_urb *urbs;
		struct usbfs_urb **iso_urbs;
	};

	enum reap_action reap_action;
	int num_urbs;
	int num_retired;
	enum libusb_transfer_status reap_status;

	/* next iso packet in user-supplied transfer to be populated */
	int iso_packet_offset;
};

static void _get_usbfs_path(struct libusb_device *dev, char *path)
{
	if (usbdev_names)
		snprintf(path, PATH_MAX, "%s/usbdev%d.%d",
			usbfs_path, dev->bus_number, dev->device_address);
	else
		snprintf(path, PATH_MAX, "%s/%03d/%03d",
			usbfs_path, dev->bus_number, dev->device_address);
}

static struct linux_device_priv *_device_priv(struct libusb_device *dev)
{
	return (struct linux_device_priv *) dev->os_priv;
}

static struct linux_device_handle_priv *_device_handle_priv(
	struct libusb_device_handle *handle)
{
	return (struct linux_device_handle_priv *) handle->os_priv;
}

/* check dirent for a /dev/usbdev%d.%d name
 * optionally return bus/device on success */
static int _is_usbdev_entry(struct dirent *entry, int *bus_p, int *dev_p)
{
	int busnum, devnum;

	if (sscanf(entry->d_name, "usbdev%d.%d", &busnum, &devnum) != 2)
		return 0;

	usbi_dbg("found: %s", entry->d_name);
	if (bus_p != NULL)
		*bus_p = busnum;
	if (dev_p != NULL)
		*dev_p = devnum;
	return 1;
}

static int check_usb_vfs(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	int found = 0;

	dir = opendir(dirname);
	if (!dir)
		return 0;

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;

		/* We assume if we find any files that it must be the right place */
		found = 1;
		break;
	}

	closedir(dir);
	return found;
}

static const char *find_usbfs_path(void)
{
	const char *path = "/dev/bus/usb";
	const char *ret = NULL;

	if (check_usb_vfs(path)) {
		ret = path;
	} else {
		path = "/proc/bus/usb";
		if (check_usb_vfs(path))
			ret = path;
	}

	/* look for /dev/usbdev*.* if the normal places fail */
	if (ret == NULL) {
		struct dirent *entry;
		DIR *dir;

		path = "/dev";
		dir = opendir(path);
		if (dir != NULL) {
			while ((entry = readdir(dir)) != NULL) {
				if (_is_usbdev_entry(entry, NULL, NULL)) {
					/* found one; that's enough */
					ret = path;
					usbdev_names = 1;
					break;
				}
			}
			closedir(dir);
		}
	}

	if (ret != NULL)
		usbi_dbg("found usbfs at %s", ret);

	return ret;
}

/* the monotonic clock is not usable on all systems (e.g. embedded ones often
 * seem to lack it). fall back to REALTIME if we have to. */
static clockid_t find_monotonic_clock(void)
{
#ifdef CLOCK_MONOTONIC
	struct timespec ts;
	int r;

	/* Linux 2.6.28 adds CLOCK_MONOTONIC_RAW but we don't use it
	 * because it's not available through timerfd */
	r = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (r == 0)
		return CLOCK_MONOTONIC;
	usbi_dbg("monotonic clock doesn't work, errno %d", errno);
#endif

	return CLOCK_REALTIME;
}

static int kernel_version_ge(int major, int minor, int sublevel)
{
	struct utsname uts;
	int atoms, kmajor, kminor, ksublevel;

	if (uname(&uts) < 0)
		return -1;
	atoms = sscanf(uts.release, "%d.%d.%d", &kmajor, &kminor, &ksublevel);
	if (atoms < 1)
		return -1;

	if (kmajor > major)
		return 1;
	if (kmajor < major)
		return 0;

	/* kmajor == major */
	if (atoms < 2)
		return 0 == minor && 0 == sublevel;
	if (kminor > minor)
		return 1;
	if (kminor < minor)
		return 0;

	/* kminor == minor */
	if (atoms < 3)
		return 0 == sublevel;

	return ksublevel >= sublevel;
}

/* Return 1 if filename exists inside dirname in sysfs.
   SYSFS_DEVICE_PATH is assumed to be the beginning of the path. */
static int sysfs_has_file(const char *dirname, const char *filename)
{
	struct stat statbuf;
	char path[PATH_MAX];
	int r;

	snprintf(path, PATH_MAX, "%s/%s/%s", SYSFS_DEVICE_PATH, dirname, filename);
	r = stat(path, &statbuf);
	if (r == 0 && S_ISREG(statbuf.st_mode))
		return 1;

	return 0;
}

static int op_init(struct libusb_context *ctx)
{
	struct stat statbuf;
	int r;

	usbfs_path = find_usbfs_path();
	if (!usbfs_path) {
		usbi_err(ctx, "could not find usbfs");
		return LIBUSB_ERROR_OTHER;
	}

	if (monotonic_clkid == -1)
		monotonic_clkid = find_monotonic_clock();

	if (supports_flag_bulk_continuation == -1) {
		/* bulk continuation URB flag available from Linux 2.6.32 */
		supports_flag_bulk_continuation = kernel_version_ge(2,6,32);
		if (supports_flag_bulk_continuation == -1) {
			usbi_err(ctx, "error checking for bulk continuation support");
			return LIBUSB_ERROR_OTHER;
		}
	}

	if (supports_flag_bulk_continuation)
		usbi_dbg("bulk continuation flag supported");

	if (-1 == supports_flag_zero_packet) {
		/* zero length packet URB flag fixed since Linux 2.6.31 */
		supports_flag_zero_packet = kernel_version_ge(2,6,31);
		if (-1 == supports_flag_zero_packet) {
			usbi_err(ctx, "error checking for zero length packet support");
			return LIBUSB_ERROR_OTHER;
		}
	}

	if (supports_flag_zero_packet)
		usbi_dbg("zero length packet flag supported");

	r = stat(SYSFS_DEVICE_PATH, &statbuf);
	if (r == 0 && S_ISDIR(statbuf.st_mode)) {
		DIR *devices = opendir(SYSFS_DEVICE_PATH);
		struct dirent *entry;

		usbi_dbg("found usb devices in sysfs");

		if (!devices) {
			usbi_err(ctx, "opendir devices failed errno=%d", errno);
			return LIBUSB_ERROR_IO;
		}

		/* Make sure sysfs supports all the required files. If it
		 * does not, then usbfs will be used instead.  Determine
		 * this by looping through the directories in
		 * SYSFS_DEVICE_PATH.  With the assumption that there will
		 * always be subdirectories of the name usbN (usb1, usb2,
		 * etc) representing the root hubs, check the usbN
		 * subdirectories to see if they have all the needed files.
		 * This algorithm uses the usbN subdirectories (root hubs)
		 * because a device disconnection will cause a race
		 * condition regarding which files are available, sometimes
		 * causing an incorrect result.  The root hubs are used
		 * because it is assumed that they will always be present.
		 * See the "sysfs vs usbfs" comment at the top of this file
		 * for more details.  */
		while ((entry = readdir(devices))) {
			int has_busnum=0, has_devnum=0, has_descriptors=0;
			int has_configuration_value=0;

			/* Only check the usbN directories. */
			if (strncmp(entry->d_name, "usb", 3) != 0)
				continue;

			/* Check for the files libusb needs from sysfs. */
			has_busnum = sysfs_has_file(entry->d_name, "busnum");
			has_devnum = sysfs_has_file(entry->d_name, "devnum");
			has_descriptors = sysfs_has_file(entry->d_name, "descriptors");
			has_configuration_value = sysfs_has_file(entry->d_name, "bConfigurationValue");

			if (has_busnum && has_devnum && has_configuration_value)
				sysfs_can_relate_devices = 1;
			if (has_descriptors)
				sysfs_has_descriptors = 1;

			/* Only need to check until we've found ONE device which
			   has all the attributes. */
			if (sysfs_has_descriptors && sysfs_can_relate_devices)
				break;
		}
		closedir(devices);

		/* Only use sysfs descriptors if the rest of
		   sysfs will work for libusb. */
		if (!sysfs_can_relate_devices)
			sysfs_has_descriptors = 0;
	} else {
		usbi_dbg("sysfs usb info not available");
		sysfs_has_descriptors = 0;
		sysfs_can_relate_devices = 0;
	}

        pthread_mutex_lock(&hotplug_lock);
        if (!init_count++) {
                /* start up hotplug event handler */
                r = linux_start_event_monitor();
                if (LIBUSB_SUCCESS != r) {
                        usbi_err(ctx, "error starting hotplug event monitor");
                        return r;
                }
        }
        pthread_mutex_unlock(&hotplug_lock);

        r = linux_scan_devices(ctx);
        if (LIBUSB_SUCCESS != r) {
                return r;
        }

	return r;
}

static void op_exit(void)
{
        if (!init_count) {
                /* should not happen */
		return;
        }

        pthread_mutex_lock(&hotplug_lock);
        if (!--init_count) {
                /* tear down event handler */
                (void)linux_stop_event_monitor();
        }
        pthread_mutex_unlock(&hotplug_lock);
}

static int linux_start_event_monitor(void)
{
#if defined(USE_UDEV)
        return linux_udev_start_event_monitor();
#else
        return linux_netlink_start_event_monitor();
#endif
}

static int linux_stop_event_monitor(void)
{
#if defined(USE_UDEV)
        return linux_udev_stop_event_monitor();
#else
        return linux_netlink_stop_event_monitor();
#endif
}

static int linux_scan_devices(struct libusb_context *ctx)
{
#if defined(USE_UDEV)
        return linux_udev_scan_devices(ctx);
#else
        return linux_default_scan_devices(ctx);
#endif
}

static int usbfs_get_device_descriptor(struct libusb_device *dev,
	unsigned char *buffer)
{
	struct linux_device_priv *priv = _device_priv(dev);

	/* return cached copy */
	memcpy(buffer, priv->dev_descriptor, DEVICE_DESC_LENGTH);
	return 0;
}

static int _open_sysfs_attr(struct libusb_device *dev, const char *attr)
{
	struct linux_device_priv *priv = _device_priv(dev);
	char filename[PATH_MAX];
	int fd;

	snprintf(filename, PATH_MAX, "%s/%s/%s",
		SYSFS_DEVICE_PATH, priv->sysfs_dir, attr);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		usbi_err(DEVICE_CTX(dev),
			"open %s failed ret=%d errno=%d", filename, fd, errno);
		return LIBUSB_ERROR_IO;
	}

	return fd;
}

/* Note only suitable for attributes which always read >= 0, < 0 is error */
static int __read_sysfs_attr(struct libusb_context *ctx,
	const char *devname, const char *attr)
{
	char filename[PATH_MAX];
	FILE *f;
	int r, value;

	snprintf(filename, PATH_MAX, "%s/%s/%s", SYSFS_DEVICE_PATH,
		 devname, attr);
	f = fopen(filename, "r");
	if (f == NULL) {
		if (errno == ENOENT) {
			/* File doesn't exist. Assume the device has been
			   disconnected (see trac ticket #70). */
			return LIBUSB_ERROR_NO_DEVICE;
		}
		usbi_err(ctx, "open %s failed errno=%d", filename, errno);
		return LIBUSB_ERROR_IO;
	}

	r = fscanf(f, "%d", &value);
	fclose(f);
	if (r != 1) {
		usbi_err(ctx, "fscanf %s returned %d, errno=%d", attr, r, errno);
		return LIBUSB_ERROR_NO_DEVICE; /* For unplug race (trac #70) */
	}
	if (value < 0) {
		usbi_err(ctx, "%s contains a negative value", filename);
		return LIBUSB_ERROR_IO;
	}

	return value;
}

static int sysfs_get_device_descriptor(struct libusb_device *dev,
	unsigned char *buffer)
{
	int fd;
	ssize_t r;

	/* sysfs provides access to an in-memory copy of the device descriptor,
	 * so we use that rather than keeping our own copy */

	fd = _open_sysfs_attr(dev, "descriptors");
	if (fd < 0)
		return fd;

	r = read(fd, buffer, DEVICE_DESC_LENGTH);;
	close(fd);
	if (r < 0) {
		usbi_err(DEVICE_CTX(dev), "read failed, ret=%d errno=%d", fd, errno);
		return LIBUSB_ERROR_IO;
	} else if (r < DEVICE_DESC_LENGTH) {
		usbi_err(DEVICE_CTX(dev), "short read %d/%d", r, DEVICE_DESC_LENGTH);
		return LIBUSB_ERROR_IO;
	}

	return 0;
}

static int op_get_device_descriptor(struct libusb_device *dev,
	unsigned char *buffer, int *host_endian)
{
	if (sysfs_has_descriptors) {
		*host_endian = 0;
		return sysfs_get_device_descriptor(dev, buffer);
	} else {
		*host_endian = 1;
		return usbfs_get_device_descriptor(dev, buffer);
	}
}

static int usbfs_get_active_config_descriptor(struct libusb_device *dev,
	unsigned char *buffer, size_t len)
{
	struct linux_device_priv *priv = _device_priv(dev);
	if (!priv->config_descriptor)
		return LIBUSB_ERROR_NOT_FOUND; /* device is unconfigured */

	/* retrieve cached copy */
	memcpy(buffer, priv->config_descriptor, len);
	return 0;
}

/* read the bConfigurationValue for a device */
static int sysfs_get_active_config(struct libusb_device *dev, int *config)
{
	char *endptr;
	char tmp[4] = {0, 0, 0, 0};
	long num;
	int fd;
	ssize_t r;

	fd = _open_sysfs_attr(dev, "bConfigurationValue");
	if (fd < 0)
		return fd;

	r = read(fd, tmp, sizeof(tmp));
	close(fd);
	if (r < 0) {
		usbi_err(DEVICE_CTX(dev), 
			"read bConfigurationValue failed ret=%d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	} else if (r == 0) {
		usbi_dbg("device unconfigured");
		*config = -1;
		return 0;
	}

	if (tmp[sizeof(tmp) - 1] != 0) {
		usbi_err(DEVICE_CTX(dev), "not null-terminated?");
		return LIBUSB_ERROR_IO;
	} else if (tmp[0] == 0) {
		usbi_err(DEVICE_CTX(dev), "no configuration value?");
		return LIBUSB_ERROR_IO;
	}

	num = strtol(tmp, &endptr, 10);
	if (endptr == tmp) {
		usbi_err(DEVICE_CTX(dev), "error converting '%s' to integer", tmp);
		return LIBUSB_ERROR_IO;
	}

	*config = (int) num;
	return 0;
}

/* takes a usbfs/descriptors fd seeked to the start of a configuration, and
 * seeks to the next one. */
static int seek_to_next_config(struct libusb_context *ctx, int fd,
	int host_endian)
{
	struct libusb_config_descriptor config;
	unsigned char tmp[6];
	off_t off;
	ssize_t r;

	/* read first 6 bytes of descriptor */
	r = read(fd, tmp, sizeof(tmp));
	if (r < 0) {
		usbi_err(ctx, "read failed ret=%d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	} else if (r < (ssize_t)sizeof(tmp)) {
		usbi_err(ctx, "short descriptor read %d/%d", r, sizeof(tmp));
		return LIBUSB_ERROR_IO;
	}

	/* seek forward to end of config */
	usbi_parse_descriptor(tmp, "bbwbb", &config, host_endian);
	off = lseek(fd, config.wTotalLength - sizeof(tmp), SEEK_CUR);
	if (off < 0) {
		usbi_err(ctx, "seek failed ret=%d errno=%d", off, errno);
		return LIBUSB_ERROR_IO;
	}

	return 0;
}

static int sysfs_get_active_config_descriptor(struct libusb_device *dev,
	unsigned char *buffer, size_t len)
{
	int fd;
	ssize_t r;
	off_t off;
	int to_copy;
	int config;
	unsigned char tmp[6];

	r = sysfs_get_active_config(dev, &config);
	if (r < 0)
		return r;
	if (config == -1)
		return LIBUSB_ERROR_NOT_FOUND;

	usbi_dbg("active configuration %d", config);

	/* sysfs provides access to an in-memory copy of the device descriptor,
	 * so we use that rather than keeping our own copy */

	fd = _open_sysfs_attr(dev, "descriptors");
	if (fd < 0)
		return fd;

	/* device might have been unconfigured since we read bConfigurationValue,
	 * so first check that there is any config descriptor data at all... */
	off = lseek(fd, 0, SEEK_END);
	if (off < 1) {
		usbi_err(DEVICE_CTX(dev), "end seek failed, ret=%d errno=%d",
			off, errno);
		close(fd);
		return LIBUSB_ERROR_IO;
	} else if (off == DEVICE_DESC_LENGTH) {
		close(fd);
		return LIBUSB_ERROR_NOT_FOUND;
	}

	off = lseek(fd, DEVICE_DESC_LENGTH, SEEK_SET);
	if (off < 0) {
		usbi_err(DEVICE_CTX(dev), "seek failed, ret=%d errno=%d", off, errno);
		close(fd);
		return LIBUSB_ERROR_IO;
	}

	/* unbounded loop: we expect the descriptor to be present under all
	 * circumstances */
	while (1) {
		r = read(fd, tmp, sizeof(tmp));
		if (r < 0) {
			usbi_err(DEVICE_CTX(dev), "read failed, ret=%d errno=%d",
				fd, errno);
			return LIBUSB_ERROR_IO;
		} else if (r < (ssize_t)sizeof(tmp)) {
			usbi_err(DEVICE_CTX(dev), "short read %d/%d", r, sizeof(tmp));
			return LIBUSB_ERROR_IO;
		}

		/* check bConfigurationValue */
		if (tmp[5] == config)
			break;

		/* try the next descriptor */
		off = lseek(fd, 0 - sizeof(tmp), SEEK_CUR);
		if (off < 0)
			return LIBUSB_ERROR_IO;

		r = seek_to_next_config(DEVICE_CTX(dev), fd, 0);
		if (r < 0)
			return r;
	}

	to_copy = (len < sizeof(tmp)) ? len : sizeof(tmp);
	memcpy(buffer, tmp, to_copy);
	if (len > sizeof(tmp)) {
		r = read(fd, buffer + sizeof(tmp), len - sizeof(tmp));
		if (r < 0) {
			usbi_err(DEVICE_CTX(dev), "read failed, ret=%d errno=%d",
				fd, errno);
			r = LIBUSB_ERROR_IO;
		} else if (r == 0) {
			usbi_dbg("device is unconfigured");
			r = LIBUSB_ERROR_NOT_FOUND;
		} else if ((size_t)r < len - sizeof(tmp)) {
			usbi_err(DEVICE_CTX(dev), "short read %d/%d", r, len);
			r = LIBUSB_ERROR_IO;
		}
	} else {
		r = 0;
	}

	close(fd);
	return r;
}

int linux_get_device_address (struct libusb_context *ctx, int detached,
			      uint8_t *busnum, uint8_t *devaddr,
			      const char *dev_node, const char *sys_name)
{
	int retbus, retdev;

	usbi_dbg("getting address for device: %s detached: %d",
		 sys_name, detached);
        /* can't use sysfs to read the bus and device number if the
           device has been detached */
        if (!sysfs_can_relate_devices || detached || NULL == sys_name) {
		if (NULL == dev_node) {
			return LIBUSB_ERROR_OTHER;
		}

                /* will this work with all supported kernel versions? */
                if (!strncmp(dev_node, "/dev/bus/usb", 12)) {
                        sscanf (dev_node, "/dev/bus/usb/%hhd/%hhd", busnum, devaddr);
                } else if (!strncmp(dev_node, "/proc/bus/usb", 13)) {
                        sscanf (dev_node, "/proc/bus/usb/%hhd/%hhd", busnum, devaddr);
                }

                return LIBUSB_SUCCESS;
        }

	usbi_dbg("scan %s", sys_name);

	*busnum = retbus = __read_sysfs_attr(ctx, sys_name, "busnum");
	if (retbus < 0)
		return retbus;
                
        *devaddr = retdev = __read_sysfs_attr(ctx, sys_name, "devnum");
        if (retdev < 0)
                return retdev;

	usbi_dbg("bus=%d dev=%d", *busnum, *devaddr);
	if (retbus > 255 || retdev > 255)
		return LIBUSB_ERROR_INVALID_PARAM;

        return LIBUSB_SUCCESS;
}

static int op_get_active_config_descriptor(struct libusb_device *dev,
	unsigned char *buffer, size_t len, int *host_endian)
{
	*host_endian = *host_endian;
	if (sysfs_has_descriptors) {
		return sysfs_get_active_config_descriptor(dev, buffer, len);
	} else {
		return usbfs_get_active_config_descriptor(dev, buffer, len);
	}
}

/* takes a usbfs fd, attempts to find the requested config and copy a certain
 * amount of it into an output buffer. */
static int get_config_descriptor(struct libusb_context *ctx, int fd,
	uint8_t config_index, unsigned char *buffer, size_t len)
{
	off_t off;
	ssize_t r;

	off = lseek(fd, DEVICE_DESC_LENGTH, SEEK_SET);
	if (off < 0) {
		usbi_err(ctx, "seek failed ret=%d errno=%d", off, errno);
		return LIBUSB_ERROR_IO;
	}

	/* might need to skip some configuration descriptors to reach the
	 * requested configuration */
	while (config_index > 0) {
		r = seek_to_next_config(ctx, fd, 1);
		if (r < 0)
			return r;
		config_index--;
	}

	/* read the rest of the descriptor */
	r = read(fd, buffer, len);
	if (r < 0) {
		usbi_err(ctx, "read failed ret=%d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	} else if ((size_t)r < len) {
		usbi_err(ctx, "short output read %d/%d", r, len);
		return LIBUSB_ERROR_IO;
	}

	return 0;
}

static int op_get_config_descriptor(struct libusb_device *dev,
	uint8_t config_index, unsigned char *buffer, size_t len, int *host_endian)
{
	char filename[PATH_MAX];
	int fd;
	int r;

	*host_endian = *host_endian;
	/* always read from usbfs: sysfs only has the active descriptor
	 * this will involve waking the device up, but oh well! */

	/* FIXME: the above is no longer true, new kernels have all descriptors
	 * in the descriptors file. but its kinda hard to detect if the kernel
	 * is sufficiently new. */

	_get_usbfs_path(dev, filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		usbi_err(DEVICE_CTX(dev),
			"open '%s' failed, ret=%d errno=%d", filename, fd, errno);
		return LIBUSB_ERROR_IO;
	}

	r = get_config_descriptor(DEVICE_CTX(dev), fd, config_index, buffer, len);
	close(fd);
	return r;
}

/* cache the active config descriptor in memory. a value of -1 means that
 * we aren't sure which one is active, so just assume the first one. 
 * only for usbfs. */
static int cache_active_config(struct libusb_device *dev, int fd,
	int active_config)
{
	struct linux_device_priv *priv = _device_priv(dev);
	struct libusb_config_descriptor config;
	unsigned char tmp[8];
	unsigned char *buf;
	int idx;
	int r;

	if (active_config == -1) {
		idx = 0;
	} else {
		r = usbi_get_config_index_by_value(dev, active_config, &idx);
		if (r < 0)
			return r;
		if (idx == -1)
			return LIBUSB_ERROR_NOT_FOUND;
	}

	r = get_config_descriptor(DEVICE_CTX(dev), fd, idx, tmp, sizeof(tmp));
	if (r < 0) {
		usbi_err(DEVICE_CTX(dev), "first read error %d", r);
		return r;
	}

	usbi_parse_descriptor(tmp, "bbw", &config, 0);
	buf = malloc(config.wTotalLength);
	if (!buf)
		return LIBUSB_ERROR_NO_MEM;

	r = get_config_descriptor(DEVICE_CTX(dev), fd, idx, buf,
		config.wTotalLength);
	if (r < 0) {
		free(buf);
		return r;
	}

	if (priv->config_descriptor)
		free(priv->config_descriptor);
	priv->config_descriptor = buf;
	return 0;
}

/* send a control message to retrieve active configuration */
static int usbfs_get_active_config(struct libusb_device *dev, int fd)
{
	unsigned char active_config = 0;
	int r;

	struct usbfs_ctrltransfer ctrl = {
		.bmRequestType = LIBUSB_ENDPOINT_IN,
		.bRequest = LIBUSB_REQUEST_GET_CONFIGURATION,
		.wValue = 0,
		.wIndex = 0,
		.wLength = 1,
		.timeout = 1000,
		.data = &active_config
	};

	r = ioctl(fd, IOCTL_USBFS_CONTROL, &ctrl);
	if (r < 0) {
		if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		/* we hit this error path frequently with buggy devices :( */
		usbi_warn(DEVICE_CTX(dev),
			"get_configuration failed ret=%d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	}

	return active_config;
}

static int initialize_device(struct libusb_device *dev, uint8_t busnum,
	uint8_t devaddr, const char *sysfs_dir)
{
	struct linux_device_priv *priv = _device_priv(dev);
	unsigned char *dev_buf;
	char path[PATH_MAX];
	int fd, speed;
	int active_config = 0;
	int device_configured = 1;
	ssize_t r;

	dev->bus_number = busnum;
	dev->device_address = devaddr;

	if (sysfs_dir) {
		priv->sysfs_dir = malloc(strlen(sysfs_dir) + 1);
		if (!priv->sysfs_dir)
			return LIBUSB_ERROR_NO_MEM;
		strcpy(priv->sysfs_dir, sysfs_dir);

		/* Note speed can contain 1.5, in this case __read_sysfs_attr
		   will stop parsing at the '.' and return 1 */
		speed = __read_sysfs_attr(DEVICE_CTX(dev), sysfs_dir, "speed");
		if (speed >= 0) {
			switch (speed) {
			case     1: dev->speed = LIBUSB_SPEED_LOW; break;
			case    12: dev->speed = LIBUSB_SPEED_FULL; break;
			case   480: dev->speed = LIBUSB_SPEED_HIGH; break;
			case  5000: dev->speed = LIBUSB_SPEED_SUPER; break;
			default:
				usbi_warn(DEVICE_CTX(dev), "Unknown device speed: %d Mbps", speed);
			}
		}
	}

	if (sysfs_has_descriptors)
		return 0;

	/* cache device descriptor in memory so that we can retrieve it later
	 * without waking the device up (op_get_device_descriptor) */

	priv->dev_descriptor = NULL;
	priv->config_descriptor = NULL;

	if (sysfs_can_relate_devices) {
		int tmp = sysfs_get_active_config(dev, &active_config);
		if (tmp < 0)
			return tmp;
		if (active_config == -1)
			device_configured = 0;
	}

	_get_usbfs_path(dev, path);
	fd = open(path, O_RDWR);
	if (fd < 0 && errno == EACCES) {
		fd = open(path, O_RDONLY);
		/* if we only have read-only access to the device, we cannot
		 * send a control message to determine the active config. just
		 * assume the first one is active. */
		active_config = -1;
	}

	if (fd < 0) {
		usbi_err(DEVICE_CTX(dev), "open failed, ret=%d errno=%d", fd, errno);
		return LIBUSB_ERROR_IO;
	}

	if (!sysfs_can_relate_devices) {
		if (active_config == -1) {
			/* if we only have read-only access to the device, we cannot
			 * send a control message to determine the active config. just
			 * assume the first one is active. */
			usbi_warn(DEVICE_CTX(dev), "access to %s is read-only; cannot "
				"determine active configuration descriptor", path);
		} else {
			active_config = usbfs_get_active_config(dev, fd);
			if (active_config == LIBUSB_ERROR_IO) {
				/* buggy devices sometimes fail to report their active config.
				 * assume unconfigured and continue the probing */
				usbi_warn(DEVICE_CTX(dev), "couldn't query active "
					"configuration, assumung unconfigured");
				device_configured = 0;
			} else if (active_config < 0) {
				close(fd);
				return active_config;
			} else if (active_config == 0) {
				/* some buggy devices have a configuration 0, but we're
				 * reaching into the corner of a corner case here, so let's
				 * not support buggy devices in these circumstances.
				 * stick to the specs: a configuration value of 0 means
				 * unconfigured. */
				usbi_dbg("active cfg 0? assuming unconfigured device");
				device_configured = 0;
			}
		}
	}

	dev_buf = malloc(DEVICE_DESC_LENGTH);
	if (!dev_buf) {
		close(fd);
		return LIBUSB_ERROR_NO_MEM;
	}

	r = read(fd, dev_buf, DEVICE_DESC_LENGTH);
	if (r < 0) {
		usbi_err(DEVICE_CTX(dev),
			"read descriptor failed ret=%d errno=%d", fd, errno);
		free(dev_buf);
		close(fd);
		return LIBUSB_ERROR_IO;
	} else if (r < DEVICE_DESC_LENGTH) {
		usbi_err(DEVICE_CTX(dev), "short descriptor read (%d)", r);
		free(dev_buf);
		close(fd);
		return LIBUSB_ERROR_IO;
	}

	/* bit of a hack: set num_configurations now because cache_active_config()
	 * calls usbi_get_config_index_by_value() which uses it */
	dev->num_configurations = dev_buf[DEVICE_DESC_LENGTH - 1];

	if (device_configured) {
		r = cache_active_config(dev, fd, active_config);
		if (r < 0) {
			close(fd);
			free(dev_buf);
			return r;
		}
	}

	close(fd);
	priv->dev_descriptor = dev_buf;
	return 0;
}

int linux_enumerate_device(struct libusb_context *ctx,
		     uint8_t busnum, uint8_t devaddr,
		     const char *sysfs_dir)
{
	unsigned long session_id;
	struct libusb_device *dev;
	int r = 0;

	/* FIXME: session ID is not guaranteed unique as addresses can wrap and
	 * will be reused. instead we should add a simple sysfs attribute with
	 * a session ID. */
	session_id = busnum << 8 | devaddr;
	usbi_dbg("busnum %d devaddr %d session_id %ld", busnum, devaddr,
		session_id);

	usbi_dbg("allocating new device for %d/%d (session %ld)",
		 busnum, devaddr, session_id);
	dev = usbi_alloc_device(ctx, session_id);
	if (!dev)
		return LIBUSB_ERROR_NO_MEM;

	r = initialize_device(dev, busnum, devaddr, sysfs_dir);
	if (r < 0)
		goto out;
	r = usbi_sanitize_device(dev);
	if (r < 0)
		goto out;
out:
	if (r < 0)
		libusb_unref_device(dev);
	else
		usbi_connect_device(dev);

	return r;
}

void linux_hotplug_enumerate(uint8_t busnum, uint8_t devaddr, const char *sys_name)
{
	struct libusb_context *ctx;

	list_for_each_entry(ctx, &active_contexts_list, list, struct libusb_context) {
		if (usbi_get_device_by_session_id(ctx, busnum << 8 | devaddr)) {
			/* device already exists in the context */
			usbi_dbg("device already exists in context");
			continue;
		}

		linux_enumerate_device(ctx, busnum, devaddr, sys_name);
	}
}

void linux_hotplug_disconnected(uint8_t busnum, uint8_t devaddr, const char *sys_name)
{
	struct libusb_context *ctx;
	struct libusb_device *dev;

	list_for_each_entry(ctx, &active_contexts_list, list, struct libusb_context) {
		dev = usbi_get_device_by_session_id (ctx, busnum << 8 | devaddr);
		if (NULL != dev) {
			usbi_disconnect_device (dev);
		} else {
			usbi_err(ctx, "device not found for session %x %s", busnum << 8 | devaddr, sys_name);
		}
	}
}

#if !defined(USE_UDEV)
/* open a bus directory and adds all discovered devices to the context */
static int usbfs_scan_busdir(struct libusb_context *ctx, uint8_t busnum)
{
	DIR *dir;
	char dirpath[PATH_MAX];
	struct dirent *entry;
	int r = LIBUSB_ERROR_IO;

	snprintf(dirpath, PATH_MAX, "%s/%03d", usbfs_path, busnum);
	usbi_dbg("%s", dirpath);
	dir = opendir(dirpath);
	if (!dir) {
		usbi_err(ctx, "opendir '%s' failed, errno=%d", dirpath, errno);
		/* FIXME: should handle valid race conditions like hub unplugged
		 * during directory iteration - this is not an error */
		return r;
	}

	while ((entry = readdir(dir))) {
		int devaddr;

		if (entry->d_name[0] == '.')
			continue;

		devaddr = atoi(entry->d_name);
		if (devaddr == 0) {
			usbi_dbg("unknown dir entry %s", entry->d_name);
			continue;
		}

		if (linux_enumerate_device(ctx, busnum, (uint8_t) devaddr, NULL)) {
			usbi_dbg("failed to enumerate dir entry %s", entry->d_name);
			continue;
		}

		r = 0;
	}

	closedir(dir);
	return r;
}

static int usbfs_get_device_list(struct libusb_context *ctx)
{
	struct dirent *entry;
	DIR *buses = opendir(usbfs_path);
	int r = 0;

	if (!buses) {
		usbi_err(ctx, "opendir buses failed errno=%d", errno);
		return LIBUSB_ERROR_IO;
	}

	while ((entry = readdir(buses))) {
		int busnum;

		if (entry->d_name[0] == '.')
			continue;

		if (usbdev_names) {
			int devaddr;
			if (!_is_usbdev_entry(entry, &busnum, &devaddr))
				continue;

			r = linux_enumerate_device(ctx, busnum, (uint8_t) devaddr, NULL);
			if (r < 0) {
				usbi_dbg("failed to enumerate dir entry %s", entry->d_name);
				continue;
			}
		} else {
			busnum = atoi(entry->d_name);
			if (busnum == 0) {
				usbi_dbg("unknown dir entry %s", entry->d_name);
				continue;
			}

			r = usbfs_scan_busdir(ctx, busnum);
			if (r < 0)
				break;
		}
	}

	closedir(buses);
	return r;

}

static int sysfs_scan_device(struct libusb_context *ctx, const char *devname)
{
	uint8_t busnum, devaddr;
	int ret;

	ret = linux_get_device_address (ctx, 0, &busnum, &devaddr, NULL, devname);
	if (LIBUSB_SUCCESS != ret) {
		return ret;
	}

	return linux_enumerate_device(ctx, busnum & 0xff, devaddr & 0xff,
		devname);
}

static int sysfs_get_device_list(struct libusb_context *ctx)
{
	DIR *devices = opendir(SYSFS_DEVICE_PATH);
	struct dirent *entry;
	int r = LIBUSB_ERROR_IO;

	if (!devices) {
		usbi_err(ctx, "opendir devices failed errno=%d", errno);
		return r;
	}

	while ((entry = readdir(devices))) {
		if ((!isdigit(entry->d_name[0]) && strncmp(entry->d_name, "usb", 3))
				|| strchr(entry->d_name, ':'))
			continue;

		if (sysfs_scan_device(ctx, entry->d_name)) {
			usbi_dbg("failed to enumerate dir entry %s", entry->d_name);
			continue;
		}

		r = 0;
	}

	closedir(devices);
	return r;
}

static int linux_default_scan_devices (struct libusb_context *ctx)
{
	/* we can retrieve device list and descriptors from sysfs or usbfs.
	 * sysfs is preferable, because if we use usbfs we end up resuming
	 * any autosuspended USB devices. however, sysfs is not available
	 * everywhere, so we need a usbfs fallback too.
	 *
	 * as described in the "sysfs vs usbfs" comment at the top of this
	 * file, sometimes we have sysfs but not enough information to
	 * relate sysfs devices to usbfs nodes.  op_init() determines the
	 * adequacy of sysfs and sets sysfs_can_relate_devices.
	 */
	if (sysfs_can_relate_devices != 0)
		return sysfs_get_device_list(ctx);
	else
		return usbfs_get_device_list(ctx);
}
#endif

static int op_open(struct libusb_device_handle *handle)
{
	struct linux_device_handle_priv *hpriv = _device_handle_priv(handle);
	char filename[PATH_MAX];
	int r;

	_get_usbfs_path(handle->dev, filename);
	usbi_dbg("opening %s", filename);
	hpriv->fd = open(filename, O_RDWR);
	if (hpriv->fd < 0) {
		if (errno == EACCES) {
			usbi_err(HANDLE_CTX(handle), "libusb couldn't open USB device %s: "
				"Permission denied.", filename);
			usbi_err(HANDLE_CTX(handle),
				"libusb requires write access to USB device nodes.");
			return LIBUSB_ERROR_ACCESS;
		} else if (errno == ENOENT) {
			usbi_err(HANDLE_CTX(handle), "libusb couldn't open USB device %s: "
				"No such file or directory.", filename);
			return LIBUSB_ERROR_NO_DEVICE;
		} else {
			usbi_err(HANDLE_CTX(handle),
				"open failed, code %d errno %d", hpriv->fd, errno);
			return LIBUSB_ERROR_IO;
		}
	}

	r = ioctl(hpriv->fd, IOCTL_USBFS_GET_CAPABILITIES, &hpriv->caps);
	if (r < 0) {
		if (errno == ENOTTY)
			usbi_dbg("%s: getcap not available", filename);
		else
			usbi_err(HANDLE_CTX(handle),
				 "%s: getcap failed (%d)", filename, errno);
		hpriv->caps = 0;
		if (supports_flag_zero_packet)
			hpriv->caps |= USBFS_CAP_ZERO_PACKET;
		if (supports_flag_bulk_continuation)
			hpriv->caps |= USBFS_CAP_BULK_CONTINUATION;
	}

	return usbi_add_pollfd(HANDLE_CTX(handle), hpriv->fd, POLLOUT);
}

static void op_close(struct libusb_device_handle *dev_handle)
{
	int fd = _device_handle_priv(dev_handle)->fd;
	usbi_remove_pollfd(HANDLE_CTX(dev_handle), fd);
	close(fd);
}

static int op_get_configuration(struct libusb_device_handle *handle,
	int *config)
{
	int r;
	if (sysfs_can_relate_devices != 1)
		return LIBUSB_ERROR_NOT_SUPPORTED;

	r = sysfs_get_active_config(handle->dev, config);
	if (r < 0)
		return r;

	if (*config == -1) {
		usbi_err(HANDLE_CTX(handle), "device unconfigured");
		*config = 0;
	}

	return 0;
}

static int op_set_configuration(struct libusb_device_handle *handle, int config)
{
	struct linux_device_priv *priv = _device_priv(handle->dev);
	int fd = _device_handle_priv(handle)->fd;
	int r = ioctl(fd, IOCTL_USBFS_SETCONFIG, &config);
	if (r) {
		if (errno == EINVAL)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == EBUSY)
			return LIBUSB_ERROR_BUSY;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle), "failed, error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	if (!sysfs_has_descriptors) {
		/* update our cached active config descriptor */
		if (config == -1) {
			if (priv->config_descriptor) {
				free(priv->config_descriptor);
				priv->config_descriptor = NULL;
			}
		} else {
			r = cache_active_config(handle->dev, fd, config);
			if (r < 0)
				usbi_warn(HANDLE_CTX(handle),
					"failed to update cached config descriptor, error %d", r);
		}
	}

	return 0;
}

static int op_claim_interface(struct libusb_device_handle *handle, int iface)
{
	int fd = _device_handle_priv(handle)->fd;
	int r = ioctl(fd, IOCTL_USBFS_CLAIMINTF, &iface);
	if (r) {
		if (errno == ENOENT)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == EBUSY)
			return LIBUSB_ERROR_BUSY;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"claim interface failed, error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}
	return 0;
}

static int op_release_interface(struct libusb_device_handle *handle, int iface)
{
	int fd = _device_handle_priv(handle)->fd;
	int r = ioctl(fd, IOCTL_USBFS_RELEASEINTF, &iface);
	if (r) {
		if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"release interface failed, error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}
	return 0;
}

static int op_set_interface(struct libusb_device_handle *handle, int iface,
	int altsetting)
{
	int fd = _device_handle_priv(handle)->fd;
	struct usbfs_setinterface setintf;
	int r;

	setintf.interface = iface;
	setintf.altsetting = altsetting;
	r = ioctl(fd, IOCTL_USBFS_SETINTF, &setintf);
	if (r) {
		if (errno == EINVAL)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"setintf failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	return 0;
}

static int op_clear_halt(struct libusb_device_handle *handle,
	unsigned char endpoint)
{
	int fd = _device_handle_priv(handle)->fd;
	unsigned int _endpoint = endpoint;
	int r = ioctl(fd, IOCTL_USBFS_CLEAR_HALT, &_endpoint);
	if (r) {
		if (errno == ENOENT)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"clear_halt failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	return 0;
}

static int op_reset_device(struct libusb_device_handle *handle)
{
	int fd = _device_handle_priv(handle)->fd;
	int i, r, ret = 0;

	/* Doing a device reset will cause the usbfs driver to get unbound
	   from any interfaces it is bound to. By voluntarily unbinding
	   the usbfs driver ourself, we stop the kernel from rebinding
	   the interface after reset (which would end up with the interface
	   getting bound to the in kernel driver if any). */
	for (i = 0; i < USB_MAXINTERFACES; i++) {
		if (handle->claimed_interfaces & (1L << i)) {
			op_release_interface(handle, i);
		}
	}

	usbi_mutex_lock(&handle->lock);
	r = ioctl(fd, IOCTL_USBFS_RESET, NULL);
	if (r) {
		if (errno == ENODEV) {
			ret = LIBUSB_ERROR_NOT_FOUND;
			goto out;
		}

		usbi_err(HANDLE_CTX(handle),
			"reset failed error %d errno %d", r, errno);
		ret = LIBUSB_ERROR_OTHER;
		goto out;
	}

	/* And re-claim any interfaces which were claimed before the reset */
	for (i = 0; i < USB_MAXINTERFACES; i++) {
		if (handle->claimed_interfaces & (1L << i)) {
			r = op_claim_interface(handle, i);
			if (r) {
				usbi_warn(HANDLE_CTX(handle),
					"failed to re-claim interface %d after reset", i);
				handle->claimed_interfaces &= ~(1L << i);
			}
		}
	}
out: