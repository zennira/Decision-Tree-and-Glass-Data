/*
 * Copyright 2013 Andrew Smith
 * Copyright 2013 Con Kolivas
 * Copyright 2013 Chris Savery
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
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#include "config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "compat.h"
#include "miner.h"
#include "usbutils.h"

#define K1 "K1"
#define K16 "K16"
#define K64 "K64"

static const char *msg_detect_send = "DSend";
static const char *msg_detect_reply = "DReply";
static const char *msg_send = "Send";
static const char *msg_reply = "Reply";

#define KLN_CMD_ABORT	'A'
#define KLN_CMD_CONFIG	'C'
#define KLN_CMD_ENABLE	'E'
#define KLN_CMD_IDENT	'I'
#define KLN_CMD_NONCE	'='
#define KLN_CMD_STATUS	'S'
#define KLN_CMD_WORK	'W'

#define KLN_CMD_ENABLE_OFF	'0'
#define KLN_CMD_ENABLE_ON	'1'

#define MIDSTATE_BYTES 32
#define MERKLE_OFFSET 64
#define MERKLE_BYTES 12

#define REPLY_SIZE		15	// adequate for all types of replies
#define MAX_KLINES		1024	// unhandled reply limit
#define REPLY_WAIT_TIME		100 	// poll interval for a cmd waiting it's reply
#define CMD_REPLY_RETRIES	8	// how many retries for cmds
#define MAX_WORK_COUNT		4	// for now, must be binary multiple and match firmware
#define TACH_FACTOR		87890	// fan rpm divisor

#define KLN_KILLWORK_TEMP	53.5
#define KLN_COOLED_DOWN		45.5

/*
 *  Work older than 5s will already be completed
 *  FYI it must not be possible to complete 256 work
 *  items this quickly on a single device -
 *  thus limited to 219.9GH/s per device
 */
#define OLD_WORK_MS ((int)(5 * 1000))

/*
 * How many incorrect slave counts to ignore in a row
 * 2 means it allows random grabage returned twice
 * Until slaves are implemented, this should never occur
 * so allowing 2 in a row should ignore random errros
 */
#define KLN_ISS_IGNORE 2

/*
 * If the queue status hasn't been updated for this long then do it now
 * 5GH/s = 859ms per full nonce range
 */
#define LATE_UPDATE_MS ((int)(2.5 * 1000))

// If 5 late updates in a row, try to reset the device
#define LATE_UPDATE_LIMIT	5

// If the reset fails sleep for 1s
#define LATE_UPDATE_SLEEP_MS 1000

// However give up after 8s
#define LATE_UPDATE_NODEV_MS ((int)(8.0 * 1000))

struct device_drv klondike_drv;

typedef struct klondike_header {
	uint8_t cmd;
	uint8_t dev;
	uint8_t buf[REPLY_SIZE-2];
} HEADER;

#define K_2(_bytes) ((int)(_bytes[0]) + \
			((int)(_bytes[1]) << 8))

#define K_4(_bytes) ((uint64_t)(_bytes[0]) + \
			((uint64_t)(_bytes[1]) << 8) + \
			((uint64_t)(_bytes[2]) << 16) + \
			((uint64_t)(_bytes[3]) << 24))

#define K_SERIAL(_serial) K_4(_serial)
#define K_HASHCOUNT(_hashcount) K_2(_hashcount)
#define K_MAXCOUNT(_maxcount) K_2(_maxcount)
#define K_NONCE(_nonce) K_4(_nonce)
#define K_HASHCLOCK(_hashclock) K_2(_hashclock)

#define SET_HASHCLOCK(_hashclock, _value) do { \
						(_hashclock)[0] = (uint8_t)((_value) & 0xff); \
						(_hashclock)[1] = (uint8_t)(((_value) >> 8) & 0xff); \
					  } while(0)

#define KSENDHD(_add) (sizeof(uint8_t) + sizeof(uint8_t) + _add)

typedef struct klondike_id {
	uint8_t cmd;
	uint8_t dev;
	uint8_t version;
	uint8_t product[7];
	uint8_t serial[4];
} IDENTITY;

typedef struct klondike_status {
	uint8_t cmd;
	uint8_t dev;
	uint8_t state;
	uint8_t chipcount;
	ui