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
		bitforce->kname = KNAME_RANGE;
	} else {
		bitforce->sleep_ms = BITFORCE_SLEEP_MS * 5;
		bitforce->kname = KNAME_WORK;
	}

	if (!add_cgpu(bitforce))
		goto unshin;

	update_usb_stats(bitforce);

	mutex_init(&bitforce->device_mutex);

	return true;

unshin:

	usb_uninit(bitforce);

shin:

	if (bitforce->name != blank) {
		free(bitforce->name);
		bitforce->name = NULL;
	}

	bitforce = usb_free_cgpu(bitforce);

	return false;
}

static void bitforce_detect(bool __maybe_unused hotplug)
{
	usb_detect(&bitforce_drv, bitforce_detect_one);
}

static void get_bitforce_statline_before(char *buf, size_t bufsiz, struct cgpu_info *bitforce)
{
	float gt = bitforce->temp;

	if (gt > 0)
		tailsprintf(buf, bufsiz, "%5.1fC ", gt);
	else
		tailsprintf(buf, bufsiz, "       ");

	tailsprintf(buf, bufsiz, "        | ");
}

static bool bitforce_thread_prepare(__maybe_unused struct thr_info *thr)
{
//	struct cgpu_info *bitforce = thr->cgpu;

	return true;
}

static void bitforce_flash_led(struct cgpu_info *bitforce)
{
	int err, amount;

	/* Do not try to flash the led if we're polling for a result to
	 * minimise the chance of interleaved results */
	if (bitforce->polling)
		return;

	/* It is not critical flashing the led so don't get stuck if we
	 * can't grab the mutex now */
	if (mutex_trylock(&bitforce->device_mutex))
		return;

	if ((err = usb_write(bitforce, BITFORCE_FLASH, BITFORCE_FLASH_LEN, &amount, C_REQUESTFLASH)) < 0 || amount != BITFORCE_FLASH_LEN) {
		applog(LOG_ERR, "%s%i: flash request failed (%d:%d)",
			bitforce->drv->name, bitforce->device_id, amount, err);
	} else {
		/* However, this stops anything else getting a reply
		 * So best to delay any other access to the BFL */
		cgsleep_ms(4000);
	}

	/* Once we've tried - don't do it until told to again */
	bitforce->flash_led = false;

	mutex_unlock(&bitforce->device_mutex);

	return; // nothing is returned by the BFL
}

static bool bitforce_get_temp(struct cgpu_info *bitforce)
{
	char buf[BITFORCE_BUFSIZ+1];
	int err, amount;
	char *s;

	// Device is gone
	if (bitforce->usbinfo.nodev)
		return false;

	/* Do not try to get the temperature if we're polling for a result to
	 * minimise the chance of interleaved results */
	if (bitforce->polling)
		return true;

	// Flash instead of Temp - doing both can be too slow
	if (bitforce->flash_led) {
		bitforce_flash_led(bitforce);
 		return true;
	}

	/* It is not critical getting temperature so don't get stuck if we
	 * can't grab the mutex here */
	if (mutex_trylock(&bitforce->device_mutex))
		return false;

	if ((err = usb_write(bitforce, BITFORCE_TEMPERATURE, BITFORCE_TEMPERATURE_LEN, &amount, C_REQUESTTEMPERATURE)) < 0 || amount != BITFORCE_TEMPERATURE_LEN) {
		mutex_unlock(&bitforce->device_mutex);
		applog(LOG_ERR, "%s%i: Error: Request temp invalid/timed out (%d:%d)",
				bitforce->drv->name, bitforce->device_id, amount, err);
		bitforce->hw_errors++;
		return false;
	}

	if ((err = usb_read_nl(bitforce, buf, sizeof(buf)-1, &amount, C_GETTEMPERATURE)) < 0 || amount < 1) {
		mutex_unlock(&bitforce->device_mutex);
		if (err < 0) {
			applog(LOG_ERR, "%s%i: Error: Get temp return invalid/timed out (%d:%d)",
					bitforce->drv->name, bitforce->device_id, amount, err);
		} else {
			applog(LOG_ERR, "%s%i: Error: Get temp returned nothing (%d:%d)",
					bitforce->drv->name, bitforce->device_id, amount, err);
		}
		bitforce->hw_errors++;
		return false;
	}

	mutex_unlock(&bitforce->device_mutex);
	
	if ((!strncasecmp(buf, "TEMP", 4)) && (s = strchr(buf + 4, ':'))) {
		float temp = strtof(s + 1, NULL);

		/* Cope with older software  that breaks and reads nonsense
		 * values */
		if (temp > 100)
			temp = strtod(s + 1, NULL);

		if (temp > 0) {
			bitforce->temp = temp;
			if (unlikely(bitforce->cutofftemp > 0 && temp > bitforce->cutofftemp)) {
				applog(LOG_WARNING, "%s%i: Hit thermal cutoff limit, disabling!",
							bitforce->drv->name, bitforce->device_id);
				bitforce->deven = DEV_RECOVER;
				dev_error(bitforce, REASON_DEV_THERMAL_CUTOFF);
			}
		}
	} else {
		/* Use the temperature monitor as a kind of watchdog for when
		 * our responses are out of sync and flush the buffer to
		 * hopefully recover */
		applog(LOG_WARNING, "%s%i: Garbled response probably throttling, clearing buffer",
					bitforce->drv->name, bitforce->device_id);
		dev_error(bitforce, REASON_DEV_THROTTLE);
		/* Count throttling episodes as hardware errors */
		bitforce->hw_errors++;
		bitforce_initialise(bitforce, true);
		return false;
	}

	return true;
}

static bool bitforce_send_work(struct thr_info *thr, struct work *work)
{
	struct cgpu_info *bitforce = thr->cgpu;
	unsigned char ob[70];
	char buf[BITFORCE_BUFSIZ+1];
	int err, amount;
	char *s;
	char *cmd;
	int len;

re_send:
	if (bitforce->nonce_range) {
		cmd = BITFORCE_SENDRANGE;
		len = BITFORCE_SENDRANGE_LEN;
	} else {
		cmd = BITFORCE_SENDWORK;
		len = BITFORCE_SENDWORK_LEN;
	}

	mutex_lock(&bitforce->device_mutex);
	if ((err = usb_write(bitforce, cmd, len, &amount, C_REQUESTSENDWORK)) < 0 || amount != len) {
		mutex_unlock(&bitforce->device_mutex);
		applog(LOG_ERR, "%s%i: request send work failed (%d:%d)",
				bitforce->drv->name, bitforce->device_id, amount, err);
		return false;
	}

	if ((err = usb_read_nl(bitforce, buf, sizeof(buf)-1, &amount, C_REQUESTSENDWORKSTATUS)) < 0) {
		mutex_unlock(&bitforce->device_mutex);
		applog(LOG_ERR, "%s%d: read request send work status failed (%d:%d)",
				bitforce->drv->name, bitforce->device_id, amount, err);
		return false;
	}

	if (amount == 0 || !buf[0] || !strncasecmp(buf, "B", 1)) {
		mutex_unlock(&bitforce->device_mutex);
		cgsleep_ms(WORK_CHECK_INTERVAL_MS);
		goto re_send;
	} else if (unlikely(strncasecmp(buf, "OK", 2))) {
		mutex_unlock(&bitforce->device_mutex);
		if (bitforce->nonce_range) {
			applog(LOG_WARNING, "%s%i: Does not support nonce range, disabling",
						bitforce->drv->name, bitforce->device_id);
			bitforce->nonce_range = false;
			bitforce->sleep_ms *= 5;
			bitforce->kname = KNAME_WORK;
			goto re_send;
		}
		applog(LOG_ERR, "%s%i: Error: Send work reports: %s",
				bitforce->drv->name, bitforce->device_id, buf);
		return false;
	}

	sprintf((char *)ob, ">>>>>>>>");
	memcpy(ob + 8, work->midstate, 32);
	memcpy(ob + 8 + 32, work->data + 64, 12);
	if (!bitforce->nonce_range) {
		sprintf((char *)ob + 8 + 32 + 12, ">>>>>>>>