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

// Either of Nonce or No-n