/*
 * Copyright 2013 Andrew Smith
 * Copyright 2013 Con Kolivas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include <float.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "compat.h"
#include "miner.h"
#include "usbutils.h"
#include "driver-bflsc.h"

int opt_bflsc_overheat = BFLSC_TEMP_OVERHEAT;

static const char *blank = "";

static enum driver_version drv_ver(struct cgpu_info *bflsc, const char *ver)
{
	char *tmp;

	if (strstr(ver, "1.0.0"))
		return BFLSC_DRV1;

	if (strstr(ver, "1.0.") || strstr(ver, "1.1.")) {
		applog(LOG_WARNING, "%s detect (%s) Warning assuming firmware '%s' is Ver1",
			bflsc->drv->dname, bflsc->device_path, ver);
		return BFLSC_DRV1;
	}

	if (strstr(ver, "1.2."))
		return BFLSC_DRV2;

	tmp = str_text((char *)ver);
	applog(LOG_WARNING, "%s detect (%s) Warning unknown firmware '%s' using Ver2",
		bflsc->drv->dname, bflsc->device_path, tmp);
	free(tmp);
	return BFLSC_DRV2;
}

static void xlinkstr(char *xlink, size_t siz, int dev, struct bflsc_info *sc_info)
{
	if (dev > 0)
		snprintf(xlink, siz, " x-%d", dev);
	else {
		if (sc_info->sc_count > 1)
			strcpy(xlink, " master");
		else
			*xlink = '\0';
	}
}

static void bflsc_applog(struct cgpu_info *bflsc, int dev, enum usb_cmds cmd, int amount, int err)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	char xlink[17];

	xlinkstr(xlink, sizeof(xlink), dev, sc_info);

	usb_applog(bflsc, cmd, xlink, amount, err);
}

// Break an input up into lines with LFs removed
// false means an error, but if *lines > 0 then data was also found
// error would be no data or missing LF at the end
static bool tolines(struct cgpu_info *bflsc, int dev, char *buf, int *lines, char ***items, enum usb_cmds cmd)
{
	bool ok = false;
	char *ptr;

#define p_lines (*lines)
#define p_items (*items)

	p_lines = 0;
	p_items = NULL;

	if (!buf || !(*buf)) {
		applog(LOG_DEBUG, "USB: %s%i: (%d) empty %s",
			bflsc->drv->name, bflsc->device_id, dev, usb_cmdname(cmd));
		return ok;
	}

	ptr = strdup(buf);
	while (ptr && *ptr) {
		p_items = realloc(p_items, ++p_lines * sizeof(*p_items));
		if (unlikely(!p_items))
			quit(1, "Failed to realloc p_items in tolines");
		p_items[p_lines-1] = ptr;
		ptr = strchr(ptr, '\n');
		if (ptr)
			*(ptr++) = '\0';
		else {
			applog(LOG_DEBUG, "USB: %s%i: (%d) missing lf(s) in %s",
				bflsc->drv->name, bflsc->device_id, dev, usb_cmdname(cmd));
			return ok;
		}
	}
	ok = true;

	return ok;
}

static void freetolines(int *lines, char ***items)
{
	if (*lines > 0) {
		free(**items);
		free(*items);
	}
	*lines = 0;
	*items = NULL;
}

enum breakmode {
	NOCOLON,
	ONECOLON,
	ALLCOLON // Temperature uses this
};

// Break down a single line into 'fields'
// 'lf' will be a pointer to the final LF if it is there (or NULL)
// firstname will be the allocated buf copy pointer which is also
//  the string before ':' for ONECOLON and ALLCOLON
// If any string is missing the ':' when it was expected, false is returned
static bool breakdown(enum breakmode mode, char *buf, int *count, char **firstname, char ***fields, char **lf)
{
	char *ptr, *colon, *comma;
	bool ok = false;

#define p_count (*count)
#define p_firstname (*firstname)
#define p_fields (*fields)
#define p_lf (*lf)

	p_count = 0;
	p_firstname = NULL;
	p_fields = NULL;
	p_lf = NULL;

	if (!buf || !(*buf))
		return ok;

	ptr = p_firstname = strdup(buf);
	p_lf = strchr(p_firstname, '\n');
	if (mode == ONECOLON) {
		colon = strchr(ptr, ':');
		if (colon) {
			ptr = colon;
			*(ptr++) = '\0';
		} else
			return ok;
	}

	while (ptr && *ptr) {
		if (mode == ALLCOLON) {
			colon = strchr(ptr, ':');
			if (colon)
				ptr = colon + 1;
			else
				return ok;
		}
		comma = strchr(ptr, ',');
		if (comma)
			*(comma++) = '\0';
		p_fields = realloc(p_fields, ++p_count * sizeof(*p_fields));
		if (unlikely(!p_fields))
			quit(1, "Failed to realloc p_fields in breakdown");
		p_fields[p_count-1] = ptr;
		ptr = comma;
	}

	ok = true;
	return ok;
}

static void freebreakdown(int *count, char **firstname, char ***fields)
{
	if (*firstname)
		free(*firstname);
	if (*count > 0)
		free(*fields);
	*count = 0;
	*firstname = NULL;
	*fields = NULL;
}

static bool isokerr(int err, char *buf, int amount)
{
	if (err < 0 || amount < (int)BFLSC_OK_LEN)
		return false;
	else {
		if (strstr(buf, BFLSC_ANERR))
			return false;
		else
			return true;
	}
}

// send+receive dual stage - always single line replies
static int send_recv_ds(struct cgpu_info *bflsc, int dev, int *stage, bool *sent, int *amount, char *send1, int send1_len, enum usb_cmds send1_cmd,  enum usb_cmds recv1_cmd, char *send2, int send2_len, enum usb_cmds send2_cmd, enum usb_cmds recv2_cmd, char *recv, int recv_siz)
{
	struct DataForwardToChain data;
	int len, err, tried;

	if (dev == 0) {
		usb_buffer_clear(bflsc);

		*stage = 1;
		*sent = false;
		err = usb_write(bflsc, send1, send1_len, amount, send1_cmd);
		if (err < 0 || *amount < send1_len)
			return err;

		*sent = true;
		err = usb_read_nl(bflsc, recv, recv_siz, amount, recv1_cmd);
		if (!isokerr(err, recv, *amount))
			return err;

		usb_buffer_clear(bflsc);

		*stage = 2;
		*sent = false;
		err = usb_write(bflsc, send2, send2_len, amount, send2_cmd);
		if (err < 0 || *amount < send2_len)
			return err;

		*sent = true;
		err = usb_read_nl(bflsc, recv, recv_siz, amount, recv2_cmd);

		return err;
	}

	data.header = BFLSC_XLINKHDR;
	data.deviceAddress = (uint8_t)dev;
	tried = 0;
	while (tried++ < 3) {
		data.payloadSize = send1_len;
		memcpy(data.payloadData, send1, send1_len);
		len = DATAFORWARDSIZE(data);

		usb_buffer_clear(bflsc);

		*stage = 1;
		*sent = false;
		err = usb_write(bflsc, (char *)&data, len, amount, send1_cmd);
		if (err < 0 || *amount < send1_len)
			return err;

		*sent = true;
		err = usb_read_nl(bflsc, recv, recv_siz, amount, recv1_cmd);

		if (err != LIBUSB_SUCCESS)
			return err;

		// x-link timeout? - try again?
		if (strstr(recv, BFLSC_XTIMEOUT))
			continue;

		if (!isokerr(err, recv, *amount))
			return err;

		data.payloadSize = send2_len;
		memcpy(data.payloadData, send2, send2_len);
		len = DATAFORWARDSIZE(data);

		usb_buffer_clear(bflsc);

		*stage = 2;
		*sent = false;
		err = usb_write(bflsc, (char *)&data, len, amount, send2_cmd);
		if (err < 0 || *amount < send2_len)
			return err;

		*sent = true;
		err = usb_read_nl(bflsc, recv, recv_siz, amount, recv2_cmd);

		if (err != LIBUSB_SUCCESS)
			return err;

		// x-link timeout? - try again?
		if (strstr(recv, BFLSC_XTIMEOUT))
			continue;

		// SUCCESS - return it
		break;
	}
	return err;
}

#define READ_OK true
#define READ_NL false

// send+receive single stage
static int send_recv_ss(struct cgpu_info *bflsc, int dev, bool *sent, int *amount, char *send, int send_len, enum usb_cmds send_cmd, char *recv, int recv_siz, enum usb_cmds recv_cmd, bool read_ok)
{
	struct DataForwardToChain data;
	int len, err, tried;

	if (dev == 0) {
		usb_buffer_clear(bflsc);

		*sent = false;
		err = usb_write(bflsc, send, send_len, amount, send_cmd);
		if (err < 0 || *amount < send_len) {
			// N.B. thus !(*sent) directly implies err < 0 or *amount < send_len
			return err;
		}

		*sent = true;
		if (read_ok == READ_OK)
			err = usb_read_ok(bflsc, recv, recv_siz, amount, recv_cmd);
		else
			err = usb_read_nl(bflsc, recv, recv_siz, amount, recv_cmd);

		return err;
	}

	data.header = BFLSC_XLINKHDR;
	data.deviceAddress = (uint8_t)dev;
	data.payloadSize = send_len;
	memcpy(data.payloadData, send, send_len);
	len = DATAFORWARDSIZE(data);

	tried = 0;
	while (tried++ < 3) {
		usb_buffer_clear(bflsc);

		*sent = false;
		err = usb_write(bflsc, (char *)&data, len, amount, recv_cmd);
		if (err < 0 || *amount < send_len)
			return err;

		*sent = true;
		if (read_ok == READ_OK)
			err = usb_read_ok(bflsc, recv, recv_siz, amount, recv_cmd);
		else
			err = usb_read_nl(bflsc, recv, recv_siz, amount, recv_cmd);

		if (err != LIBUSB_SUCCESS && err != LIBUSB_ERROR_TIMEOUT)
			return err;

		// read_ok can err timeout if it's looking for OK<LF>
		// TODO: add a usb_read() option to spot the ERR: and convert end=OK<LF> to just <LF>
		// x-link timeout? - try again?
		if ((err == LIBUSB_SUCCESS || (read_ok == READ_OK && err == LIBUSB_ERROR_TIMEOUT)) &&
			strstr(recv, BFLSC_XTIMEOUT))
				continue;

		// SUCCESS or TIMEOUT - return it
		break;
	}
	return err;
}

static int write_to_dev(struct cgpu_info *bflsc, int dev, char *buf, int buflen, int *amount, enum usb_cmds cmd)
{
	struct DataForwardToChain data;
	int len;

	/*
	 * The protocol is syncronous so any previous excess can be
	 * discarded and assumed corrupt data or failed USB transfers
	 */
	usb_buffer_clear(bflsc);

	if (dev == 0)
		return usb_write(bflsc, buf, buflen, amount, cmd);

	data.header = BFLSC_XLINKHDR;
	data.deviceAddress = (uint8_t)dev;
	data.payloadSize = buflen;
	memcpy(data.payloadData, buf, buflen);
	len = DATAFORWARDSIZE(data);

	return usb_write(bflsc, (char *)&data, len, amount, cmd);
}

static void bflsc_send_flush_work(struct cgpu_info *bflsc, int dev)
{
	char buf[BFLSC_BUFSIZ+1];
	int err, amount;
	bool sent;

	// Device is gone
	if (bflsc->usbinfo.nodev)
		return;

	mutex_lock(&bflsc->device_mutex);
	err = send_recv_ss(bflsc, dev, &sent, &amount,
				BFLSC_QFLUSH, BFLSC_QFLUSH_LEN, C_QUEFLUSH,
				buf, sizeof(buf)-1, C_QUEFLUSHREPLY, READ_NL);
	mutex_unlock(&bflsc->device_mutex);

	if (!sent)
		bflsc_applog(bflsc, dev, C_QUEFLUSH, amount, err);
	else {
		// TODO: do we care if we don't get 'OK'? (always will in normal processing)
	}
}

/* return True = attempted usb_read_ok()
 * set ignore to true means no applog/ignore errors */
static bool bflsc_qres(struct cgpu_info *bflsc, char *buf, size_t bufsiz, int dev, int *err, int *amount, bool ignore)
{
	bool readok = false;

	mutex_lock(&(bflsc->device_mutex));
	*err = send_recv_ss(bflsc, dev, &readok, amount,
				BFLSC_QRES, BFLSC_QRES_LEN, C_REQUESTRESULTS,
				buf, bufsiz-1, C_GETRESULTS, READ_OK);
	mutex_unlock(&(bflsc->device_mutex));

	if (!readok) {
		if (!ignore)
			bflsc_applog(bflsc, dev, C_REQUESTRESULTS, *amount, *err);

		// TODO: do what? flag as dead device?
		// count how many times it has happened and reset/fail it
		// or even make sure it is all x-link and that means device
		// has failed after some limit of this?
		// of course all other I/O must also be failing ...
	} else {
		if (*err < 0 || *amount < 1) {
			if (!ignore)
				bflsc_applog(bflsc, dev, C_GETRESULTS, *amount, *err);

			// TODO: do what? ... see above
		}
	}

	return readok;
}

static void __bflsc_initialise(struct cgpu_info *bflsc)
{
	int err, interface;

// TODO: does x-link bypass the other device FTDI? (I think it does)
//	So no initialisation required except for the master device?

	if (bflsc->usbinfo.nodev)
		return;

	interface = usb_interface(bflsc);
	// Reset
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_RESET, interface, C_RESET);

	applog(LOG_DEBUG, "%s%i: reset got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (bflsc->usbinfo.nodev)
		return;

	usb_ftdi_set_latency(bflsc);

	if (bflsc->usbinfo.nodev)
		return;

	// Set data control
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_DATA,
				FTDI_VALUE_DATA_BAS, interface, C_SETDATA);

	applog(LOG_DEBUG, "%s%i: setdata got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (bflsc->usbinfo.nodev)
		return;

	// Set the baud
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_BAUD, FTDI_VALUE_BAUD_BAS,
				(FTDI_INDEX_BAUD_BAS & 0xff00) | interface,
				C_SETBAUD);

	applog(LOG_DEBUG, "%s%i: setbaud got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (bflsc->usbinfo.nodev)
		return;

	// Set Flow Control
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_FLOW,
				FTDI_VALUE_FLOW, interface, C_SETFLOW);

	applog(LOG_DEBUG, "%s%i: setflowctrl got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (bflsc->usbinfo.nodev)
		return;

	// Set Modem Control
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_MODEM,
				FTDI_VALUE_MODEM, interface, C_SETMODEM);

	applog(LOG_DEBUG, "%s%i: setmodemctrl got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (bflsc->usbinfo.nodev)
		return;

	// Clear any sent data
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_PURGE_TX, interface, C_PURGETX);

	applog(LOG_DEBUG, "%s%i: purgetx got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (bflsc->usbinfo.nodev)
		return;

	// Clear any received data
	err = usb_transfer(bflsc, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_PURGE_RX, interface, C_PURGERX);

	applog(LOG_DEBUG, "%s%i: purgerx got err %d",
		bflsc->drv->name, bflsc->device_id, err);

	if (!bflsc->cutofftemp)
		bflsc->cutofftemp = opt_bflsc_overheat;
}

static void bflsc_initialise(struct cgpu_info *bflsc)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	char buf[BFLSC_BUFSIZ+1];
	int err, amount;
	int dev;

	mutex_lock(&(bflsc->device_mutex));
	__bflsc_initialise(bflsc);
	mutex_unlock(&(bflsc->device_mutex));

	for (dev = 0; dev < sc_info->sc_count; dev++) {
		bflsc_send_flush_work(bflsc, dev);
		bflsc_qres(bflsc, buf, sizeof(buf), dev, &err, &amount, true);
	}
}

static bool getinfo(struct cgpu_info *bflsc, int dev)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	struct bflsc_dev sc_dev;
	char buf[BFLSC_BUFSIZ+1];
	int err, amount;
	char **items, *firstname, **fields, *lf;
	bool res, ok = false;
	int i, lines, count;
	char *tmp;

	/*
	 * Kano's first dev Jalapeno output:
	 * DEVICE: BitFORCE SC<LF>
	 * FIRMWARE: 1.0.0<LF>
	 * ENGINES: 30<LF>
	 * FREQUENCY: [UNKNOWN]<LF>
	 * XLINK MODE: MASTER<LF>
	 * XLINK PRESENT: YES<LF>
	 * --DEVICES IN CHAIN: 0<LF>
	 * --CHAIN PRESENCE MASK: 00000000<LF>
	 * OK<LF>
	 */

	/*
	 * Don't use send_recv_ss() since we have a different receive timeout
	 * Also getinfo() is called multiple times if it fails anyway
	 */
	err = write_to_dev(bflsc, dev, BFLSC_DETAILS, BFLSC_DETAILS_LEN, &amount, C_REQUESTDETAILS);
	if (err < 0 || amount != BFLSC_DETAILS_LEN) {
		applog(LOG_ERR, "%s detect (%s) send details request failed (%d:%d)",
			bflsc->drv->dname, bflsc->device_path, amount, err);
		return ok;
	}

	err = usb_read_ok_timeout(bflsc, buf, sizeof(buf)-1, &amount,
				  BFLSC_INFO_TIMEOUT, C_GETDETAILS);
	if (err < 0 || amount < 1) {
		if (err < 0) {
			applog(LOG_ERR, "%s detect (%s) get details return invalid/timed out (%d:%d)",
					bflsc->drv->dname, bflsc->device_path, amount, err);
		} else {
			applog(LOG_ERR, "%s detect (%s) get details returned nothing (%d:%d)",
					bflsc->drv->dname, bflsc->device_path, amount, err);
		}
		return ok;
	}

	memset(&sc_dev, 0, sizeof(struct bflsc_dev));
	sc_info->sc_count = 1;
	res = tolines(bflsc, dev, &(buf[0]), &lines, &items, C_GETDETAILS);
	if (!res)
		return ok;

	tmp = str_text(buf);
	strncpy(sc_dev.getinfo, tmp, sizeof(sc_dev.getinfo));
	sc_dev.getinfo[sizeof(sc_dev.getinfo)-1] = '\0';
	free(tmp);

	for (i = 0; i < lines-2; i++) {
		res = breakdown(ONECOLON, items[i], &count, &firstname, &fields, &lf);
		if (lf)
			*lf = '\0';
		if (!res || count != 1) {
			tmp = str_text(items[i]);
			applogsiz(LOG_WARNING, BFLSC_APPLOGSIZ,
					"%s detect (%s) invalid details line: '%s' %d",
					bflsc->drv->dname, bflsc->device_path, tmp, count);
			free(tmp);
			dev_error(bflsc, REASON_DEV_COMMS_ERROR);
			goto mata;
		}
		if (strstr(firstname, BFLSC_DI_FIRMWARE)) {
			sc_dev.firmware = strdup(fields[0]);
			sc_info->driver_version = drv_ver(bflsc, sc_dev.firmware);
		}
		else if (strstr(firstname, BFLSC_DI_ENGINES)) {
			sc_dev.engines = atoi(fields[0]);
			if (sc_dev.engines < 1) {
				tmp = str_text(items[i]);
				applogsiz(LOG_WARNING, BFLSC_APPLOGSIZ,
						"%s detect (%s) invalid engine count: '%s'",
						bflsc->drv->dname, bflsc->device_path, tmp);
				free(tmp);
				goto mata;
			}
		}
		else if (strstr(firstname, BFLSC_DI_XLINKMODE))
			sc_dev.xlink_mode = strdup(fields[0]);
		else if (strstr(firstname, BFLSC_DI_XLINKPRESENT))
			sc_dev.xlink_present = strdup(fields[0]);
		else if (strstr(firstname, BFLSC_DI_DEVICESINCHAIN)) {
			if (fields[0][0] == '0' ||
			    (fields[0][0] == ' ' && fields[0][1] == '0'))
				sc_info->sc_count = 1;
			else
				sc_info->sc_count = atoi(fields[0]);
			if (sc_info->sc_count < 1 || sc_info->sc_count > 30) {
				tmp = str_text(items[i]);
				applogsiz(LOG_WARNING, BFLSC_APPLOGSIZ,
						"%s detect (%s) invalid x-link count: '%s'",
						bflsc->drv->dname, bflsc->device_path, tmp);
				free(tmp);
				goto mata;
			}
		}
		else if (strstr(firstname, BFLSC_DI_CHIPS))
			sc_dev.chips = strdup(fields[0]);

		freebreakdown(&count, &firstname, &fields);
	}

	if (sc_info->driver_version == BFLSC_DRVUNDEF) {
		applog(LOG_WARNING, "%s detect (%s) missing %s",
			bflsc->drv->dname, bflsc->device_path, BFLSC_DI_FIRMWARE);
		goto ne;
	}

	sc_info->sc_devs = calloc(sc_info->sc_count, sizeof(struct bflsc_dev));
	if (unlikely(!sc_info->sc_devs))
		quit(1, "Failed to calloc in getinfo");
	memcpy(&(sc_info->sc_devs[0]), &sc_dev, sizeof(sc_dev));
	// TODO: do we care about getting this info for the rest if > 0 x-link

	ok = true;
	goto ne;

mata:
	freebreakdown(&count, &firstname, &fields);
	ok = false;
ne:
	freetolines(&lines, &items);
	return ok;
}

static bool bflsc_detect_one(struct libusb_device *dev, struct usb_find_devices *found)
{
	struct bflsc_info *sc_info = NULL;
	char buf[BFLSC_BUFSIZ+1];
	int i, err, amount;
	struct timeval init_start, init_now;
	int init_sleep, init_count;
	bool ident_first, sent;
	char *newname;
	uint16_t latency;

	struct cgpu_info *bflsc = usb_alloc_cgpu(&bflsc_drv, 1);

	sc_info = calloc(1, sizeof(*sc_info));
	if (unlikely(!sc_info))
		quit(1, "Failed to calloc sc_info in bflsc_detect_one");
	// TODO: fix ... everywhere ...
	bflsc->device_data = (FILE *)sc_info;

	if (!usb_init(bflsc, dev, found))
		goto shin;

	// Allow 2 complete attempts if the 1st time returns an unrecognised reply
	ident_first = true;
retry:
	init_count = 0;
	init_sleep = REINIT_TIME_FIRST_MS;
	cgtime(&init_start);
reinit:
	__bflsc_initialise(bflsc);

	err = send_recv_ss(bflsc, 0, &sent, &amount,
				BFLSC_IDENTIFY, BFLSC_IDENTIFY_LEN, C_REQUESTIDENTIFY,
				buf, sizeof(buf)-1, C_GETIDENTIFY, READ_NL);

	if (!sent) {
		applog(LOG_ERR, "%s detect (%s) send identify request failed (%d:%d)",
			bflsc->drv->dname, bflsc->device_path, amount, err);
		goto unshin;
	}

	if (err < 0 || amount < 1) {
		init_count++;
		cgtime(&init_now);
		if (us_tdiff(&init_now, &init_start) <= REINIT_TIME_MAX) {
			if (init_count == 2) {
				applog(LOG_WARNING, "%s detect (%s) 2nd init failed (%d:%d) - retrying",
					bflsc->drv->dname, bflsc->device_path, amount, err);
			}
			cgsleep_ms(init_sleep);
			if ((init_sleep * 2) <= REINIT_TIME_MAX_MS)
				init_sleep *= 2;
			goto reinit;
		}

		if (init_count > 0)
			applog(LOG_WARNING, "%s detect (%s) init failed %d times %.2fs",
				bflsc->drv->dname, bflsc->device_path, init_count, tdiff(&init_now, &init_start));

		if (err < 0) {
			applog(LOG_ERR, "%s detect (%s) error identify reply (%d:%d)",
				bflsc->drv->dname, bflsc->device_path, amount, err);
		} else {
			applog(LOG_ERR, "%s detect (%s) empty identify reply (%d)",
				bflsc->drv->dname, bflsc->device_path, amount);
		}

		goto unshin;
	}
	buf[amount] = '\0';

	if (unlikely(!strstr(buf, BFLSC_BFLSC))) {
		applog(LOG_DEBUG, "%s detect (%s) found an FPGA '%s' ignoring",
			bflsc->drv->dname, bflsc->device_path, buf);
		goto unshin;
	}

	if (unlikely(strstr(buf, BFLSC_IDENTITY))) {
		if (ident_first) {
			applog(LOG_DEBUG, "%s detect (%s) didn't recognise '%s' trying again ...",
				bflsc->drv->dname, bflsc->device_path, buf);
			ident_first = false;
			goto retry;
		}
		applog(LOG_DEBUG, "%s detect (%s) didn't recognise '%s' on 2nd attempt",
			bflsc->drv->dname, bflsc->device_path, buf);
		goto unshin;
	}

	int tries = 0;
	while (7734) {
		if (getinfo(bflsc, 0))
			break;

		// N.B. we will get displayed errors each time it fails
		if (++tries > 2)
			goto unshin;

		cgsleep_ms(40);
	}

	switch (sc_info->driver_version) {
		case BFLSC_DRV1:
			sc_info->que_size = BFLSC_QUE_SIZE_V1;
			sc_info->que_full_enough = BFLSC_QUE_FULL_ENOUGH_V1;
			sc_info->que_watermark = BFLSC_QUE_WATERMARK_V1;
			sc_info->que_low = BFLSC_QUE_LOW_V1;
			sc_info->que_noncecount = QUE_NONCECOUNT_V1;
			sc_info->que_fld_min = QUE_FLD_MIN_V1;
			sc_info->que_fld_max = QUE_FLD_MAX_V1;
			// Only Jalapeno uses 1.0.0
			sc_info->flush_size = 1;
			break;
		case BFLSC_DRV2:
		case BFLSC_DRVUNDEF:
		default:
			sc_info->driver_version = BFLSC_DRV2;

			sc_info->que_size = BFLSC_QUE_SIZE_V2;
			sc_info->que_full_enough = BFLSC_QUE_FULL_ENOUGH_V2;
			sc_info->que_watermark = BFLSC_QUE_WATERMARK_V2;
			sc_info->que_low = BFLSC_QUE_LOW_V2;
			sc_info->que_noncecount = QUE_NONCECOUNT_V2;
			sc_info->que_fld_min = QUE_FLD_MIN_V2;
			sc_info->que_fld_max = QUE_FLD_MAX_V2;
			// TODO: this can be reduced to total chip count
			sc_info->flush_size = 16 * sc_info->sc_count;
			break;
	}

	// Set parallelization based on the getinfo() response if it is present
	if (sc_info->sc_devs[0].chips && strlen(sc_info->sc_devs[0].chips)) {
		if (strstr(sc_info->sc_devs[0].chips, BFLSC_DI_CHIPS_PARALLEL)) {
			sc_info->que_noncecount = QUE_NONCECOUNT_V2;
			sc_info->que_fld_min = QUE_FLD_MIN_V2;
			sc_info->que_fld_max = QUE_FLD_MAX_V2;
		} else {
			sc_info->que_noncecount = QUE_NONCECOUNT_V1;
			sc_info->que_fld_min = QUE_FLD_MIN_V1;
			sc_info->que_fld_max = QUE_FLD_MAX_V1;
		}
	}

	sc_info->scan_sleep_time = BAS_SCAN_TIME;
	sc_info->results_sleep_time = BFLSC_RES_TIME;
	sc_info->default_ms_work = BAS_WORK_TIME;
	latency = BAS_LATENCY;

	/* When getinfo() "FREQUENCY: [UNKNOWN]" is fixed -
	 * use 'freq * engines' to estimate.
	 * Otherwise for now: */
	newname = NULL;
	if (sc_info->sc_count > 1) {
		newname = BFLSC_MINIRIG;
		sc_info->scan_sleep_time = BAM_SCAN_TIME;
		sc_info->default_ms_work = BAM_WORK_TIME;
		bflsc->usbdev->ident = IDENT_BAM;
		latency = BAM_LATENCY;
	} else {
		if (sc_info->sc_devs[0].engines < 34) { // 16 * 2 + 2
			newname = BFLSC_JALAPENO;
			sc_info->scan_sleep_time = BAJ_SCAN_TIME;
			sc_info->default_ms_work = BAJ_WORK_TIME;
			bflsc->usbdev->ident = IDENT_BAJ;
			latency = BAJ_LATENCY;
		} else if (sc_info->sc_devs[0].engines < 130)  { // 16 * 8 + 2
			newname = BFLSC_LITTLESINGLE;
			sc_info->scan_sleep_time = BAL_SCAN_TIME;
			sc_info->default_ms_work = BAL_WORK_TIME;
			bflsc->usbdev->ident = IDENT_BAL;
			latency = BAL_LATENCY;
		}
	}

	if (latency != bflsc->usbdev->found->latency) {
		bflsc->usbdev->found->latency = latency;
		usb_ftdi_set_latency(bflsc);
	}

	for (i = 0; i < sc_info->sc_count; i++)
		sc_info->sc_devs[i].ms_work = sc_info->default_ms_work;

	if (newname) {
		if (!bflsc->drv->copy)
			bflsc->drv = copy_drv(bflsc->drv);
		bflsc->drv->name = newname;
	}

	// We have a real BFLSC!
	applog(LOG_DEBUG, "%s (%s) identified as: '%s'",
		bflsc->drv->dname, bflsc->device_path, bflsc->drv->name);

	if (!add_cgpu(bflsc))
		goto unshin;

	update_usb_stats(bflsc);

	mutex_init(&bflsc->device_mutex);
	rwlock_init(&sc_info->stat_lock);

	return true;

unshin:

	usb_uninit(bflsc);

shin:

	free(bflsc->device_data);
	bflsc->device_data = NULL;

	if (bflsc->name != blank) {
		free(bflsc->name);
		bflsc->name = NULL;
	}

	bflsc = usb_free_cgpu(bflsc);

	return false;
}

static void bflsc_detect(bool __maybe_unused hotplug)
{
	usb_detect(&bflsc_drv, bflsc_detect_one);
}

static void get_bflsc_statline_before(char *buf, size_t bufsiz, struct cgpu_info *bflsc)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	float temp = 0;
	float vcc1 = 0;
	int i;

	rd_lock(&(sc_info->stat_lock));
	for (i = 0; i < sc_info->sc_count; i++) {
		if (sc_info->sc_devs[i].temp1 > temp)
			temp = sc_info->sc_devs[i].temp1;
		if (sc_info->sc_devs[i].temp2 > temp)
			temp = sc_info->sc_devs[i].temp2;
		if (sc_info->sc_devs[i].vcc1 > vcc1)
			vcc1 = sc_info->sc_devs[i].vcc1;
	}
	rd_unlock(&(sc_info->stat_lock));

	tailsprintf(buf, bufsiz, " max%3.0fC %4.2fV | ", temp, vcc1);
}

static void flush_one_dev(struct cgpu_info *bflsc, int dev)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	struct work *work, *tmp;
	bool did = false;

	bflsc_send_flush_work(bflsc, dev);

	rd_lock(&bflsc->qlock);

	HASH_ITER(hh, bflsc->queued_work, work, tmp) {
		if (work->subid == dev) {
			// devflag is used to flag stale work
			work->devflag = true;
			did = true;
		}
	}

	rd_unlock(&bflsc->qlock);

	if (did) {
		wr_lock(&(sc_info->stat_lock));
		sc_info->sc_devs[dev].flushed = true;
		sc_info->sc_devs[dev].flush_id = sc_info->sc_devs[dev].result_id;
		sc_info->sc_devs[dev].work_queued = 0;
		wr_unlock(&(sc_info->stat_lock));
	}
}

static void bflsc_flush_work(struct cgpu_info *bflsc)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	int dev;

	for (dev = 0; dev < sc_info->sc_count; dev++)
		flush_one_dev(bflsc, dev);
}

static void bflsc_flash_led(struct cgpu_info *bflsc, int dev)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	char buf[BFLSC_BUFSIZ+1];
	int err, amount;
	bool sent;

	// Device is gone
	if (bflsc->usbinfo.nodev)
		return;

	// It is not critical flashing the led so don't get stuck if we
	// can't grab the mutex now
	if (mutex_trylock(&bflsc->device_mutex))
		return;

	err = send_recv_ss(bflsc, dev, &sent, &amount,
				BFLSC_FLASH, BFLSC_FLASH_LEN, C_REQUESTFLASH,
				buf, sizeof(buf)-1, C_FLASHREPLY, READ_NL);
	mutex_unlock(&(bflsc->device_mutex));

	if (!sent)
		bflsc_applog(bflsc, dev, C_REQUESTFLASH, amount, err);
	else {
		// Don't care
	}

	// Once we've tried - don't do it until told to again
	// - even if it failed
	sc_info->flash_led = false;

	return;
}

static bool bflsc_get_temp(struct cgpu_info *bflsc, int dev)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	struct bflsc_dev *sc_dev;
	char temp_buf[BFLSC_BUFSIZ+1];
	char volt_buf[BFLSC_BUFSIZ+1];
	char *tmp;
	int err, amount;
	char *firstname, **fields, *lf;
	char xlink[17];
	int count;
	bool res, sent;
	float temp, temp1, temp2;
	float vcc1, vcc2, vmain;

	// Device is gone
	if (bflsc->usbinfo.nodev)
		return false;

	if (dev >= sc_info->sc_count) {
		applog(LOG_ERR, "%s%i: temp invalid xlink device %d - limit %d",
			bflsc->drv->name, bflsc->device_id, dev, sc_info->sc_count - 1);
		return false;
	}

	// Flash instead of Temp
	if (sc_info->flash_led) {
		bflsc_flash_led(bflsc, dev);
		return true;
	}

	xlinkstr(xlink, sizeof(xlink), dev, sc_info);

	/* It is not very critical getting temp so don't get stuck if we
	 * can't grab the mutex here */
	if (mutex_trylock(&bflsc->device_mutex))
		return false;

	err = send_recv_ss(bflsc, dev, &sent, &amount,
				BFLSC_TEMPERATURE, BFLSC_TEMPERATURE_LEN, C_REQUESTTEMPERATURE,
				temp_buf, sizeof(temp_buf)-1, C_GETTEMPERATURE, READ_NL);
	mutex_unlock(&(bflsc->device_mutex));

	if (!sent) {
		applog(LOG_ERR, "%s%i: Error: Request%s temp invalid/timed out (%d:%d)",
				bflsc->drv->name, bflsc->device_id, xlink, amount, err);
		return false;
	} else {
		if (err < 0 || amount < 1) {
			if (err < 0) {
				applog(LOG_ERR, "%s%i: Error: Get%s temp return invalid/timed out (%d:%d)",
						bflsc->drv->name, bflsc->device_id, xlink, amount, err);
			} else {
				applog(LOG_ERR, "%s%i: Error: Get%s temp returned nothing (%d:%d)",
						bflsc->drv->name, bflsc->device_id, xlink, amount, err);
			}
			return false;
		}
	}

	// Ignore it if we can't get the V
	if (mutex_trylock(&bflsc->device_mutex))
		return false;

	err = send_recv_ss(bflsc, dev, &sent, &amount,
				BFLSC_VOLTAGE, BFLSC_VOLTAGE_LEN, C_REQUESTVOLTS,
				volt_buf, sizeof(volt_buf)-1, C_GETVOLTS, READ_NL);
	mutex_unlock(&(bflsc->device_mutex));

	if (!sent) {
		applog(LOG_ERR, "%s%i: Error: Request%s volts invalid/timed out (%d:%d)",
				bflsc->drv->name, bflsc->device_id, xlink, amount, err);
		return false;
	} else {
		if (err < 0 || amount < 1) {
			if (err < 0) {
				applog(LOG_ERR, "%s%i: Error: Get%s volt return invalid/timed out (%d:%d)",
						bflsc->drv->name, bflsc->device_id, xlink, amount, err);
			} else {
				applog(LOG_ERR, "%s%i: Error: Get%s volt returned nothing (%d:%d)",
						bflsc->drv->name, bflsc->device_id, xlink, amount, err);
			}
			return false;
		}
	}

	res = breakdown(ALLCOLON, temp_buf, &count, &firstname, &fields, &lf);
	if (lf)
		*lf = '\0';
	if (!res || count != 2 || !lf) {
		tmp = str_text(temp_buf);
		applog(LOG_WARNING, "%s%i: Invalid%s temp reply: '%s'",
				bflsc->drv->name, bflsc->device_id, xlink, tmp);
		free(tmp);
		freebreakdown(&count, &firstname, &fields);
		dev_error(bflsc, REASON_DEV_COMMS_ERROR);
		return false;
	}

	temp = temp1 = (float)atoi(fields[0]);
	temp2 = (float)atoi(fields[1]);

	freebreakdown(&count, &firstname, &fields);

	res = breakdown(NOCOLON, volt_buf, &count, &firstname, &fields, &lf);
	if (lf)
		*lf = '\0';
	if (!res || count != 3 || !lf) {
		tmp = str_text(volt_buf);
		applog(LOG_WARNING, "%s%i: Invalid%s volt reply: '%s'",
				bflsc->drv->name, bflsc->device_id, xlink, tmp);
		free(tmp);
		freebreakdown(&count, &firstname, &fields);
		dev_error(bflsc, REASON_DEV_COMMS_ERROR);
		return false;
	}

	sc_dev = &sc_info->sc_devs[dev];
	vcc1 = (float)atoi(fields[0]) / 1000.0;
	vcc2 = (float)atoi(fields[1]) / 1000.0;
	vmain = (float)atoi(fields[2]) / 1000.0;

	freebreakdown(&count, &firstname, &fields);

	if (vcc1 > 0 || vcc2 > 0 || vmain > 0) {
		wr_lock(&(sc_info->stat_lock));
		if (vcc1 > 0) {
			if (unlikely(sc_dev->vcc1 == 0))
				sc_dev->vcc1 = vcc1;
			else {
				sc_dev->vcc1 += vcc1 * 0.63;
				sc_dev->vcc1 /= 1.63;
			}
		}
		if (vcc2 > 0) {
			if (unlikely(sc_dev->vcc2 == 0))
				sc_dev->vcc2 = vcc2;
			else {
				sc_dev->vcc2 += vcc2 * 0.63;
				sc_dev->vcc2 /= 1.63;
			}
		}
		if (vmain > 0) {
			if (unlikely(sc_dev->vmain == 0))
				sc_dev->vmain = vmain;
			else {
				sc_dev->vmain += vmain * 0.63;
				sc_dev->vmain /= 1.63;
			}
		}
		wr_unlock(&(sc_info->stat_lock));
	}

	if (temp1 > 0 || temp2 > 0) {
		wr_lock(&(sc_info->stat_lock));
		if (unlikely(!sc_dev->temp1))
			sc_dev->temp1 = temp1;
		else {
			sc_dev->temp1 += temp1 * 0.63;
			sc_dev->temp1 /= 1.63;
		}
		if (unlikely(!sc_dev->temp2))
			sc_dev->temp2 = temp2;
		else {
			sc_dev->temp2 += temp2 * 0.63;
			sc_dev->temp2 /= 1.63;
		}
		if (temp1 > sc_dev->temp1_max) {
			sc_dev->temp1_max = temp1;
			sc_dev->temp1_max_time = time(NULL);
		}
		if (temp2 > sc_dev->temp2_max) {
			sc_dev->temp2_max = temp2;
			sc_dev->temp2_max_time = time(NULL);
		}

		if (unlikely(sc_dev->temp1_5min_av == 0))
			sc_dev->temp1_5min_av = temp1;
		else {
			sc_dev->temp1_5min_av += temp1 * .0042;
			sc_dev->temp1_5min_av /= 1.0042;
		}
		if (unlikely(sc_dev->temp2_5min_av == 0))
			sc_dev->temp2_5min_av = temp2;
		else {
			sc_dev->temp2_5min_av += temp2 * .0042;
			sc_dev->temp2_5min_av /= 1.0042;
		}
		wr_unlock(&(sc_info->stat_lock));

		if (temp < temp2)
			temp = temp2;

		bflsc->temp = temp;

		if (bflsc->cutofftemp > 0 && temp >= bflsc->cutofftemp) {
			applog(LOG_WARNING, "%s%i:%s temp (%.1f) hit thermal cutoff limit %d, stopping work!",
						bflsc->drv->name, bflsc->device_id, xlink,
						temp, bflsc->cutofftemp);
			dev_error(bflsc, REASON_DEV_THERMAL_CUTOFF);
			sc_dev->overheat = true;
			flush_one_dev(bflsc, dev);
			return false;
		}

		if (bflsc->cutofftemp > 0 && temp < (bflsc->cutofftemp - BFLSC_TEMP_RECOVER))
			sc_dev->overheat = false;
	}

	return true;
}

static void process_nonces(struct cgpu_info *bflsc, int dev, char *xlink, char *data, int count, char **fields, int *nonces)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	char midstate[MIDSTATE_BYTES], blockdata[MERKLE_BYTES];
	struct work *work;
	uint32_t nonce;
	int i, num, x;
	bool res;
	char *tmp;

	if (count < sc_info->que_fld_min) {
		tmp = str_text(data);
		applogsiz(LOG_INFO, BFLSC_APPLOGSIZ,
				"%s%i:%s work returned too small (%d,%s)",
				bflsc->drv->name, bflsc->device_id, xlink, count, tmp);
		free(tmp);
		inc_hw_errors(bflsc->thr[0]);
		return;
	}

	if (count > sc_info->que_fld_max) {
		applog(LOG_INFO, "%s%i:%s work returned too large (%d) processing %d anyway",
		       bflsc->drv->name, bflsc->device_id, xlink, count, sc_info->que_fld_max);
		count = sc_info->que_fld_max;
		inc_hw_errors(bflsc->thr[0]);
	}

	num = atoi(fields[sc_info->que_noncecount]);
	if (num != count - sc_info->que_fld_min) {
		tmp = str_text(data);
		applogsiz(LOG_INFO, BFLSC_APPLOGSIZ,
				"%s%i:%s incorrect data count (%d) will use %d instead from (%s)",
				bflsc->drv->name, bflsc->device_id, xlink, num,
				count - sc_info->que_fld_max, tmp);
		free(tmp);
		inc_hw_errors(bflsc->thr[0]);
	}

	memset(midstate, 0, MIDSTATE_BYTES);
	memset(blockdata, 0, MERKLE_BYTES);
	if (!hex2bin((unsigned char *)midstate, fields[QUE_MIDSTATE], MIDSTATE_BYTES) ||
	    !hex2bin((unsigned char *)blockdata, fields[QUE_BLOCKDATA], MERKLE_BYTES)) {
		applog(LOG_INFO, "%s%i:%s Failed to convert binary data to hex result - ignored",
		       bflsc->drv->name, bflsc->device_id, xlink);
		inc_hw_errors(bflsc->thr[0]);
		return;
	}

	work = take_queued_work_bymidstate(bflsc, midstate, MIDSTATE_BYTES,
					   blockdata, MERKLE_OFFSET, MERKLE_BYTES);
	if (!work) {
		if (sc_info->not_first_work) {
			applog(LOG_INFO, "%s%i:%s failed to find nonce work - can't be processed - ignored",
			       bflsc->drv->name, bflsc->device_id, xlink);
			inc_hw_errors(bflsc->thr[0]);
		}
		return;
	}

	res = false;
	x = 0;
	for (i = sc_info->que_fld_min; i < count; i++) {
		if (strlen(fields[i]) != 8) {
			tmp = str_text(data);
			applogsiz(LOG_INFO, BFLSC_APPLOGSIZ,
					"%s%i:%s invalid nonce (%s) will try to process anyway",
					bflsc->drv->name, bflsc->device_id, xlink, tmp);
			free(tmp);
		}

		hex2bin((void*)&nonce, fields[i], 4);
		nonce = htobe32(nonce);
		res = submit_nonce(bflsc->thr[0], work, nonce);
		if (res) {
			wr_lock(&(sc_info->stat_lock));
			sc_info->sc_devs[dev].nonces_found++;
			wr_unlock(&(sc_info->stat_lock));

			(*nonces)++;
			x++;
		}
	}

	wr_lock(&(sc_info->stat_lock));
	if (res)
		sc_info->sc_devs[dev].result_id++;
	if (x > QUE_MAX_RESULTS)
		x = QUE_MAX_RESULTS + 1;
	(sc_info->result_size[x])++;
	sc_info->sc_devs[dev].work_complete++;
	sc_info->sc_devs[dev].hashes_unsent += FULLNONCE;
	// If not flushed (stale)
	if (!(work->devflag))
		sc_info->sc_devs[dev].work_queued -= 1;
	wr_unlock(&(sc_info->stat_lock));

	free_work(work);
}

static int process_results(struct cgpu_info *bflsc, int dev, char *pbuf, int *nonces)
{
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	char **items, *firstname, **fields, *lf;
	int que = 0, i, lines, count;
	char *tmp, *tmp2, *buf;
	char xlink[17];
	bool res;

	*nonces = 0;

	xlinkstr(xlink, sizeof(xlink), dev, sc_info);

	buf = strdup(pbuf);
	res = tolines(bflsc, dev, buf, &lines, &items, C_GETRESULTS);
	free(buf);
	if (!res || lines < 1) {
		tmp = str_text(pbuf);
		applogsiz(LOG_ERR, BFLSC_APPLOGSIZ,
				"%s%i:%s empty result (%s) ignored",
				bflsc->drv->name, bflsc->device_id, xlink, tmp);
		free(tmp);
		goto arigatou;
	}

	if (lines < QUE_RES_LINES_MIN) {
		tmp = str_text(pbuf);
		applogsiz(LOG_ERR, BFLSC_APPLOGSIZ,
				"%s%i:%s result of %d too small (%s) ignored",
				bflsc->drv->name, bflsc->device_id, xlink, lines, tmp);
		free(tmp);
		goto arigatou;
	}

	breakdown(ONECOLON, items[1], &count, &firstname, &fields, &lf);
	if (count < 1) {
		tmp = str_text(pbuf);
		tmp2 = str_text(items[1]);
		applogsiz(LOG_ERR, BFLSC_APPLOGSIZ,
				"%s%i:%s empty result count (%s) in (%s) ignoring",
				bflsc->drv->name, bflsc->device_id, xlink, tmp2, tmp);
		free(tmp2);
		free(tmp);
		goto arigatou;
	} else if (count != 1) {
		tmp = str_text(pbuf);
		tmp2 = str_text(items[1]);
		applogsiz(LOG_ERR, BFLSC_APPLOGSIZ,
				"%s%i:%s incorrect result count %d (%s) in (%s) will try anyway",
				bflsc->drv->name, bflsc->device_id, xlink, count, tmp2, tmp);
		free(tmp2);
		free(tmp);
	}

	que = atoi(fields[0]);
	if (que != (lines - QUE_RES_LINES_MIN)) {
		i = que;
		// 1+ In case the last line isn't 'OK' - try to process it
		que = 1 + lines - QUE_RES_LINES_MIN;

		tmp = str_text(pbuf);
		tmp2 = str_text(items[0]);
		applogsiz(LOG_ERR, BFLSC_APPLOGSIZ,
				"%s%i:%s incorrect result count %d (%s) will try %d (%s)",
				bflsc->drv->name, bflsc->device_id, xlink, i, tmp2, que, tmp);
		free(tmp2);
		free(tmp);

	}

	freebreakdown(&count, &firstname, &fields);

	for (i = 0; i < que; i++) {
		res = breakdown(NOCOLON, items[i + QUE_RES_LINES_MIN - 1], &count, &firstname, &fields, &lf);
		if (likely(res))
			process_nonces(bflsc, dev, &(xlink[0]), items[i], count, fields, nonces);
		else
			applogsiz(LOG_ERR, BFLSC_APPLOGSIZ,
					"%s%i:%s failed to process nonce %s",
					bflsc->drv->name, bflsc->device_id, xlink, items[i]);
		freebreakdown(&count, &firstname, &fields);
		sc_info->not_first_work = true;
	}

arigatou:
	freetolines(&lines, &items);

	return que;
}

#define TVF(tv) ((float)((tv)->tv_sec) + ((float)((tv)->tv_usec) / 1000000.0))
#define TVFMS(tv) (TVF(tv) * 1000.0)

// Thread to simply keep looking for results
static void *bflsc_get_results(void *userdata)
{
	struct cgpu_info *bflsc = (struct cgpu_info *)userdata;
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);
	struct timeval elapsed, now;
	float oldest, f;
	char buf[BFLSC_BUFSIZ+1];
	int err, amount;
	int i, que, dev, nonces;
	bool readok;

	cgtime(&now);
	for (i = 0; i < sc_info->sc_count; i++) {
		copy_time(&(sc_info->sc_devs[i].last_check_result), &now);
		copy_time(&(sc_info->sc_devs[i].last_dev_result), &now);
		copy_time(&(sc_info->sc_devs[i].last_nonce_result), &now);
	}

	while (sc_info->shutdown == false) {
		cgtimer_t ts_start;

		if (bflsc->usbinfo.nodev)
			return NULL;

		dev = -1;
		oldest = FLT_MAX;
		cgtime(&now);

		// Find the first oldest ... that also needs checking
		for (i = 0; i < sc_info->sc_count; i++) {
			timersub(&now, &(sc_info->sc_devs[i].last_check_result), &elapsed);
			f = TVFMS(&elapsed);
			if (f < oldest && f >= sc_info->sc_devs[i].ms_work) {
				f = oldest;
				dev = i;
			}
		}

		if (bflsc->usbinfo.nodev)
			return NULL;

		cgsleep_prepare_r(&ts_start);
		if (dev == -1)
			goto utsura;

		cgtime(&(sc_info->sc_devs[dev].last_check_result));

		readok = bflsc_qres(bflsc, buf, sizeof(buf), dev, &err, &amount, false);
		if (err < 0 || (!readok && amount != BFLSC_QRES_LEN) || (readok && amount < 1)) {
			// TODO: do what else?
		} else {
			que = process_results(bflsc, dev, buf, &nonces);
			sc_info->not_first_work = true; // in case it failed processing it
			if (que > 0)
				cgtime(&(sc_info->sc_devs[dev].last_dev_result));
			if (nonces > 0)
				cgtime(&(sc_info->sc_devs[dev].last_nonce_result));

			// TODO: if not getting results ... reinit?
		}

utsura:
		cgsleep_ms_r(&ts_start, sc_info->results_sleep_time);
	}

	return NULL;
}

static bool bflsc_thread_prepare(struct thr_info *thr)
{
	struct cgpu_info *bflsc = thr->cgpu;
	struct bflsc_info *sc_info = (struct bflsc_info *)(bflsc->device_data);

	if (thr_info_create(&(sc_info->results_thr), NULL, bflsc_get_results, (void *)bflsc)) {
		applog(L