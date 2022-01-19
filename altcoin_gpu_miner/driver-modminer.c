/*
 * Copyright 2012-2013 Andrew Smith
 * Copyright 2012 Luke Dashjr
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "logging.h"
#include "miner.h"
#include "usbutils.h"
#include "fpgautils.h"
#include "util.h"

#define BITSTREAM_FILENAME "fpgaminer_top_fixed7_197MHz.ncd"
#define BISTREAM_USER_ID "\2\4$B"

#define BITSTREAM_MAGIC_0 0
#define BITSTREAM_MAGIC_1 9

#define MODMINER_CUTOFF_TEMP 60.0
#define MODMINER_OVERHEAT_TEMP 50.0
#define MODMINER_RECOVER_TEMP 46.5
#define MODMINER_TEMP_UP_LIMIT 47.0

#define MODMINER_HW_ERROR_PERCENT 0.75

// How many seconds of no nonces means there's something wrong
// First time - drop the clock and see if it revives
// Second time - (and it didn't revive) disable it
#define ITS_DEAD_JIM 300

// N.B. in the latest firmware the limit is 250
// however the voltage/temperature risks preclude that
#define MODMINER_MAX_CLOCK 230
#define MODMINER_DEF_CLOCK 200
#define MODMINER_MIN_CLOCK 160

#define MODMINER_CLOCK_UP 2
#define MODMINER_CLOCK_SET 0
#define MODMINER_CLOCK_DOWN -2
// = 0 means OVERHEAT doesn't affect the clock
#define MODMINER_CLOCK_OVERHEAT 0
#define MODMINER_CLOCK_DEAD -6
#define MODMINER_CLOCK_CUTOFF -10

// Commands
#define MODMINER_PING "\x00"
#define MODMINER_GET_VERSION "\x01"
#define MODMINER_FPGA_COUNT "\x02"
// Commands + require FPGAid
#define MODMINER_GET_IDCODE '\x03'
#define MODMINER_GET_USERCODE '\x04'
#define MODMINER_PROGRAM '\x05'
#define MODMINER_SET_CLOCK '\x06'
#define MODMINER_READ_CLOCK '\x07'
#define MODMINER_SEND_WORK '\x08'
#define MODMINER_CHECK_WORK '\x09'
// One byte temperature reply
#define MODMINER_TEMP1 '\x0a'
// Two byte temperature reply
#define MODMINER_TEMP2 '\x0d'

// +6 bytes
#define MODMINER_SET_REG '\x0b'
// +2 bytes
#define MODMINER_GET_REG '\x0c'

#define FPGAID_ALL 4

// Maximum how many good shares in a row means clock up
// 96 is ~34m22s at 200MH/s
#define MODMINER_TRY_UP 96
// Initially how many good shares in a row means clock up
// This is doubled each down clock until it reaches MODMINER_TRY_UP
// 6 is ~2m9s at 200MH/s
#define MODMINER_EARLY_UP 6
// Limit when reducing shares_to_good
#define MODMINER_MIN_BACK 12

// 45 noops sent when detecting, in case the device was left in "start job" reading
static const char NOOP[] = MODMINER_PING "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

static void do_ping(struct cgpu_info *modminer)
{
	char buf[0x100+1];
	int err, amount;

	// Don't care if it fails
	err = usb_write(modminer, (char *)NOOP, sizeof(NOOP)-1, &amount, C_PING);
	applog(LOG_DEBUG, "%s%u: flush noop got %d err %d",
		modminer->drv->name, modminer->fpgaid, amount, err);

	// Clear any outstanding data
	while ((err = usb_read_once(modminer, buf, sizeof(buf)-1, &amount, C_CLEAR)) == 0 && amount > 0)
		applog(LOG_DEBUG, "%s%u: clear got %d",
			modminer->drv->name, modminer->fpgaid, amount);

	applog(LOG_DEBUG, "%s%u: final clear got %d err %d",
		modminer->drv->name, modminer->fpgaid, amount, err);
}

static bool modminer_detect_one(struct libusb_device *dev, struct usb_find_devices *found)
{
	char buf[0x100+1];
	char *devname = NULL;
	char devpath[32];
	int err, i, amount;
	bool added = false;

	struct cgpu_info *modminer = usb_alloc_cgpu(&modminer_drv, 1);

	modminer->modminer_mutex = calloc(1, sizeof(*(modminer->modminer_mutex)));
	mutex_init(modminer->modminer_mutex);
	modminer->fpgaid = (char)0;

	if (!usb_init(modminer, dev, found))
		goto shin;

	usb_set_cps(modminer, 11520);
	usb_enable_cps(modminer);

	do_ping(modminer);

	if ((err = usb_write(modminer, MODMINER_GET_VERSION, 1, &amount, C_REQUESTVERSION)) < 0 || amount != 1) {
		applog(LOG_ERR, "%s detect (%s) send version request failed (%d:%d)",
			modminer->drv->dname, modminer->device_path, amount, err);
		goto unshin;
	}

	if ((err = usb_read_once(modminer, buf, sizeof(buf)-1, &amount, C_GETVERSION)) < 0 || amount < 1) {
		if (err < 0)
			applog(LOG_ERR, "%s detect (%s) no version reply (%d)",
				modminer->drv->dname, modminer->device_path, err);
		else
			applog(LOG_ERR, "%s detect (%s) empty version reply (%d)",
				modminer->drv->dname, modminer->device_path, amount);

		applog(LOG_DEBUG, "%s detect (%s) check the firmware",
				modminer->drv->dname, modminer->device_path);

		goto unshin;
	}
	buf[amount] = '\0';
	devname = strdup(buf);
	applog(LOG_DEBUG, "%s (%s) identified as: %s", modminer->drv->dname, modminer->device_path, devname);

	if ((err = usb_write(modminer, MODMINER_FPGA_COUNT, 1, &amount, C_REQUESTFPGACOUNT) < 0 || amount != 1)) {
		applog(LOG_ERR, "%s detect (%s) FPGA count request failed (%d:%d)",
			modminer->drv->dname, modminer->device_path, amount, err);
		goto unshin;
	}

	if ((err = usb_read(modminer, buf, 1, &amount, C_GETFPGACOUNT)) < 0 || amount != 1) {
		applog(LOG_ERR, "%s detect (%s) no FPGA count reply (%d:%d)",
			modminer->drv->dname, modminer->device_path, amount, err);
		goto unshin;
	}

	// TODO: flag it use 1 byte temp if it is an old firmware
	// can detect with modminer->cgusb->serial ?

	if (buf[0] == 0) {
		applog(LOG_ERR, "%s detect (%s) zero FPGA count from %s",
			modminer->drv->dname, modminer->device_path, devname);
		goto unshin;
	}

	if (buf[0] < 1 || buf[0] > 4) {
		applog(LOG_ERR, "%s detect (%s) invalid FPGA count (%u) from %s",
			modminer->drv->dname, modminer->device_path, buf[0], devname);
		goto unshin;
	}

	applog(LOG_DEBUG, "%s (%s) %s has %u FPGAs",
		modminer->drv->dname, modminer->device_path, devname, buf[0]);

	modminer->name = devname;

	// TODO: test with 1 board missing in the middle and each end
	// to see how that affects the sequence numbers
	for (i = 0; i < buf[0]; i++) {
		struct cgpu_info *tmp = usb_copy_cgpu(modminer);

		sprintf(devpath, "%d:%d:%d",
			(int)(modminer->usbinfo.bus_number),
			(int)(modminer->usbinfo.device_address),
			i);

		tmp->device_path = strdup(devpath);

		// Only the first copy gets the already used stats
		if (added)
			tmp->usbinfo.usbstat = USB_NOSTAT;

		tmp->fpgaid = (char)i;
		tmp->modminer_mutex = modminer->modminer_mutex;

		if (!add_cgpu(tmp)) {
			tmp = usb_free_cgpu(tmp);
			goto unshin;
		}

		update_usb_stats(tmp);

		added = true;
	}

	modminer = usb_free_cgpu(modminer);

	return true;

unshin:
	if (!added)
		usb_uninit(modminer);

shin:
	if (!added) {
		free(modminer->modminer_mutex);
		modminer->modminer_mutex = NULL;
	}

	modminer = usb_free_cgpu(modminer);

	if (added)
		return true;
	else
		return false;
}

static void modminer_detect(bool __maybe_unused hotplug)
{
	usb_detect(&modminer_drv, modminer_detect_one);
}

static bool get_expect(struct cgpu_info *modminer, FILE *f, char c)
{
	char buf;

	if (fread(&buf, 1, 1, f) != 1) {
		applog(LOG_ERR, "%s%u: Error (%d) reading bitstream (%c)",
				modminer->drv->name, modminer->device_id, errno, c);
		return false;
	}

	if (buf != c) {
		applog(LOG_ERR, "%s%u: bitstream code mismatch (%c)",
				modminer->drv->name, modminer->device_id, c);
		return false;
	}

	return true;
}

static bool get_info(struct cgpu_info *modminer, FILE *f, char *buf, int bufsiz, const char *name)
{
	unsigned char siz[2];
	int len;

	if (fread(siz, 2, 1, f) != 1) {
		applog(LOG_ERR, "%s%u: Error (%d) reading bitstream '%s' len",
			modminer->drv->name, modminer->device_id, errno, name);
		return false;
	}

	len = siz[0] * 256 + siz[1];

	if (len >= bufsiz) {
		applog(LOG_ERR, "%s%u: Bitstream '%s' len too large (%d)",
			modminer->drv->name, modminer->device_id, name, len);
		return false;
	}

	if (fread(buf, len, 1, f) != 1) {
		applog(LOG_ERR, "%s%u: Error (%d) reading bitstream '%s'",
			modminer->drv->name, modminer->device_id, errno, name);
		return false;
	}

	buf[len] = '\0';

	return true;
}

#define USE_DEFAULT_TIMEOUT 0

// mutex must always be locked before calling
static bool get_status_timeout(struct cgpu_info *modminer, char *msg, unsigned int timeout, enum usb_cmds cmd)
{
	int err, amount;
	char buf[1];

	if (timeout == USE_DEFAULT_TIMEOUT)
		err = usb_read(modminer, buf, 1, &amount, cmd);
	else
		err = usb_read_timeout(modminer, buf, 1, &amount, timeout, cmd);

	if (err < 0 || amount != 1) {
		mutex_unlock(modminer->modminer_mutex);

		applog(LOG_ERR, "%s%u: Error (%d:%d) getting %s reply",
			modminer->drv->name, modminer->device_id, amount, err, msg);

		return false;
	}

	if (buf[0] != 1) {
		mutex_unlock(modminer->modminer_mutex);

		applog(LOG_ERR, "%s%u: Error, invalid %s reply (was %d should be 1)",
			modminer->drv->name, modminer->device_id, msg, buf[0]);

		return false;
	}

	return true;
}

// mutex must always be locked before calling
static bool get_status(struct cgpu_info *modminer, char *msg, enum usb_cmds cmd)
{
	return get_status_timeout(modminer, msg, USE_DEFAULT_TIMEOUT, cmd);
}

static bool modminer_fpga_upload_bitstream(struct cgpu_info *modminer)
{
	const char *bsfile = BITSTREAM_FILENAME;
	char buf[0x100], *p;
	char devmsg[64];
	unsigned char *ubuf = (unsigned char *)buf;
	unsigned long totlen, len;
	size_t buflen, remaining;
	float nextmsg, upto;
	char fpgaid = FPGAID_ALL;
	int err, amount, tries;
	char *ptr;

	FILE *f = open_bitstream("modminer", bsfile);
	if (!f) {
		mutex_unlock(modminer->modminer_mutex);

		applog(LOG_ERR, "%s%u: Error (%d) opening bitstream file %s",
			modminer->drv->name, modminer->device_id, errno, bsfile);

		return false;
	}

	if (fread(buf, 2, 1, f) != 1) {
		mutex_unlock(modminer->modminer_mutex);

		applog(LOG_ERR, "%s%u: Error (%d) reading bitstream magic",
			modminer->drv->name, modminer->device_id, errno);

		goto dame;
	}

	if (buf[0] != BITSTREAM_MAGIC_0 || buf[1] != BITSTREAM_MAGIC_1) {
		mutex_unlock(modminer->modminer_mutex);

		applog(LOG_ERR, "%s%u: bitstream has incorrect magic (%u,%u) instead of (%u,%u)",
			modminer->drv->name, modminer->device_id,
			buf[0], buf[1],
			BITSTREAM_MAGIC_0, BITSTREAM_MAGIC_1);

		goto dame;
	}

	if (fseek(f, 11L, SEEK_CUR)) {
		mutex_unlock(modminer->modminer_mutex);

		applog(LOG_ERR, "%s%u: Error (%d) bitstream seek failed",
			modminer->drv->name, modminer->device_id, errno);

		goto dame;
	}

	if (!get_expect(modminer, f, 'a'))
		goto undame;

	if (!get_info(modminer, f, buf, sizeof(buf), "Design name"))
		goto undame;

	applog(LOG_DEBUG, "%s%u: bitstream file '%s' info:",
		modminer->drv->name, modminer->device_id, bsfile);

	applog(LOG_DEB