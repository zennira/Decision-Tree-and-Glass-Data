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
			return 