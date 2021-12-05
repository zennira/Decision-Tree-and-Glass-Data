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
	struct bflsc_info *sc_info = (struct bflsc_i