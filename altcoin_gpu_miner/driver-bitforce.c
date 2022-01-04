/*
 * Copyright 2012-2013 Andrew Smith
 * Copyright 2012 Luke Dashjr
 * Copyright 2012 Con Kolivas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "config.h"

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "compat.h"
#include "miner.h"
#include "usbutils.h"
#include "util.h"

#ifdef WIN32
#include <windows.h>
#endif /* WIN32 */

#define BITFORCE_IDENTIFY "ZGX"
#define BITFORCE_IDENTIFY_LEN (sizeof(BITFORCE_IDENTIFY)-1)
#define BITFORCE_FLASH "ZMX"
#define BITFORCE_FLASH_LEN (sizeof(BITFORCE_FLASH)-1)
#define BITFORCE_TEMPERATURE "ZLX"
#define BITFORCE_TEMPERATURE_LEN (sizeof(BITFORCE_TEMPERATURE)-1)
#define BITFORCE_SENDRANGE "ZPX"
#define BITFORCE_SENDRANGE_LEN (sizeof(BITFORCE_SENDRANGE)-1)
#define BITFORCE_SENDWORK "ZDX"
#define BITFORCE_SENDWORK_LEN (sizeof(BITFORCE_SENDWORK)-1)
#define BITFORCE_WORKSTATUS "ZFX"
#define BITFORCE_WORKSTATUS_LEN (sizeof(BITFORCE_WORKSTATUS)-1)

// Either of Nonce or No-nonce start with:
#define BITFORCE_EITHER "N"
#define BITFORCE_EITHER_LEN 1
#define BITFORCE_NONCE "NONCE-FOUND"
#define BITFORCE_NONCE_LEN (sizeof(BITFORCE_NONCE)-1)
#define BITFORCE_NO_NONCE "NO-NONCE"
#define BITFORCE_NO_NONCE_MATCH 3
#define BITFORCE_IDLE "IDLE"
#define BITFORCE_IDLE_MATCH 1

#define BITFORCE_SLEEP_MS 500
#define BITFORCE_TIMEOUT_S 7
#define BITFORCE_TIMEOUT_MS (BITFORCE_TIMEOUT_S * 1000)
#define BITFORCE_LONG_TIMEOUT_S 30
#define BITFORCE_LONG_TIMEOUT_MS (BITFORCE_LONG_TIMEOUT_S * 1000)
#define BITFORCE_CHECK_INTERVAL_MS 10
#define WORK_CHECK_INTERVAL_MS 50
#define MAX_START_DELAY_MS 100
#define tv_to_ms(tval) (tval.tv_sec * 1000 + tval.tv_usec / 1000)
#define TIME_AVG_CONSTANT 8

#define KNAME_WORK  "full work"
#define KNAME_RANGE "nonce range"

#define BITFORCE_BUFSIZ (0x200)

// If initialisation fails the first time,
// sleep this amount (ms) and try again
#define REINIT_TIME_FIRST_MS 100
// Max ms per sleep
#define REINIT_TIME_MAX_MS 800
// Keep trying up to this many us
#define REINIT_TIME_MAX 3000000

static const char *blank = "";

static void bitforce_initialise(struct cgpu_info *bitforce, bool lock)
{
	int err, interface;

	if (lock)
		mutex_lock(&bitforce->device_mutex);

	if (bitforce->usbinfo.nodev)
		goto failed;

	interface = usb_interface(bitforce);
	// Reset
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_RESET, interface, C_RESET);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: reset got err %d",
			bitforce->drv->name, bitforce->device_id, err);

	if (bitforce->usbinfo.nodev)
		goto failed;

	// Set data control
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_DATA,
				FTDI_VALUE_DATA_BFL, interface, C_SETDATA);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: setdata got err %d",
			bitforce->drv->name, bitforce->device_id, err);

	if (bitforce->usbinfo.nodev)
		goto failed;

	// Set the baud
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_BAUD, FTDI_VALUE_BAUD_BFL,
				(FTDI_INDEX_BAUD_BFL & 0xff00) | interface,
				C_SETBAUD);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: setbaud got err %d",
			bitforce->drv->name, bitforce->device_id, err);

	if (bitforce->usbinfo.nodev)
		goto failed;

	// Set Flow Control
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_FLOW,
				FTDI_VALUE_FLOW, interface, C_SETFLOW);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: setflowctrl got err %d",
			bitforce->drv->name, bitforce->device_id, err);

	if (bitforce->usbinfo.nodev)
		goto failed;

	// Set Modem Control
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_MODEM,
				FTDI_VALUE_MODEM, interface, C_SETMODEM);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: setmodemctrl got err %d",
			bitforce->drv->name, bitforce->device_id, err);

	if (bitforce->usbinfo.nodev)
		goto failed;

	// Clear any sent data
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_PURGE_TX, interface, C_PURGETX);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: purgetx got err %d",
			bitforce->drv->name, bitforce->device_id, err);

	if (bitforce->usbinfo.nodev)
		goto failed;

	// Clear any received data
	err = usb_transfer(bitforce, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_PURGE_RX, interface, C_PURGERX);
	if (opt_debug)
		applog(LOG_DEBUG, "%s%i: purgerx got err %d",
			bitforce->drv->name, bitforce->device_id, err);

failed:

	if (lock)
		mutex_unlock(&bitforce->device_mutex);
}

static bool bitforce_detect_one(struct libusb_device *dev, struct usb_find_devices *found)
{
	char buf[BITFORCE_BUFSIZ+1];
	int err, amount;
	char *s;
	struct timeval init_start, init_now;
	int init_sleep, init_count;
	bool ident_first;

	struct cgpu_info *bitforce = usb_alloc_cgpu(&bitforce_drv, 1);

	if (!usb_init(bitforce, dev, found))
		goto shin;

	// Allow 2 complete attempts if the 1st time returns an unrecognised reply
	ident_first = true;
retry:
	init_count = 0;
	init_sleep = REINIT_TIME_FIRST_MS;
	cgtime(&init_start);
reinit:
	bitforce_initialise(bitforce, false);
	if ((err = usb_write(bitforce, BITFORCE_IDENTIFY, BITFORCE_IDENTIFY_LEN, &amount, C_REQUESTIDENTIFY)) < 0 || amount != BITFORCE_IDENTIFY_LEN) {
		applog(LOG_ERR, "%s detect (%s) send identify request failed (%d:%d)",
			bitforce->drv->dname, bitforce->device_path, amount, err);
		goto unshin;
	}

	if ((err = usb_read_nl(bitforce, buf, sizeof(buf)-1, &amount, C_GETIDENTIFY)) < 0 || amount < 1) {
		init_count++;
		cgtime(&init_now);
		if (us_tdiff(&init_now, &init_start) <= REINIT_TIME_MAX) {
			if (init_count == 2) {
				applog(LOG_WARNING, "%s detect (%s) 2nd init failed (%d:%d) - retrying",
					bitforce->drv->dname, bitforce->device_path, amount, err);
			}
			cgsleep_ms(init_sleep);
			if ((init_sleep * 2) <= REINIT_TIME_MAX_MS)
				init_sleep *= 2;
			goto reinit;
		}

		if (init_count > 0)
			applog(LOG_WARNING, "%s detect (%s) init failed %d times %.2fs",
				bitforce->drv->dname, bitforce->device_path, init_count, tdiff(&init_now, &init_start));

		if (err < 0) {
			applog(LOG_ERR, "%s detect (%s) error identify reply (%d:%d)",
				bitforce->drv->dname, bitforce->device_path, amount, err);
		} else {
			applog(LOG_ERR, "%s detect (%s) empty identify reply (%d)",
				bitforce->drv->dname, bitforce->device_path, amount);
		}

		goto unshin;
	}
	buf[amount] = '\0';

	if (unlikely(!strstr(buf, "SHA256"))) {
		if (ident_first) {
			applog(LOG_WARNING, "%s detect (%s) didn't recognise '%s' trying again ...",
				bitforce->drv->dname, bitforce->device_path, buf);
			ident_first = false;
			goto retry;
		}
		applog(LOG_ERR, "%s detect (%s) didn't recognise '%s' on 2nd attempt",
			bitforce->drv->dname, bitforce->device_path, buf);
		goto unshin;
	}

	if (strstr(buf, "SHA256 SC")) {
#ifdef USE_BFLSC
		applog(LOG_DEBUG, "SC device detected, will defer to BFLSC driver");
#else
		applog(LOG_WARNING, "SC device detected but no BFLSC support compiled in!");
#endif
		goto unshin;
	}

	if (likely((!memcmp(buf, ">>>ID: ", 7)) && (s = strstr(buf + 3, ">>>")))) {
		s[0] = '\0';
		bitforce->name = strdup(buf + 7);
	} else {
		bitforce->name = (char *)blank;
	}

	// We have a real BitForce!
	applog(LOG_DEBUG, "%s (%s) identified as: '%s'",
		bitforce->drv->dname, bitforce->device_path, bitforce->name);

	/* Initially enable support for nonce range and disable it later if it
	 * fails */
	if (opt_bfl_noncerange) {
		bitforce->nonce_range = true;
		bitforce->sleep_ms = BITFORCE_SLEEP_MS;
		bitforce->