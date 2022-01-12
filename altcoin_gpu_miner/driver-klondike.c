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
	uint8_t slavecount;
	uint8_t workqc;
	uint8_t workid;
	uint8_t temp;
	uint8_t fanspeed;
	uint8_t errorcount;
	uint8_t hashcount[2];
	uint8_t maxcount[2];
	uint8_t noise;
} WORKSTATUS;

typedef struct _worktask {
	uint8_t cmd;
	uint8_t dev;
	uint8_t workid;
	uint8_t midstate[32];
	uint8_t merkle[12];
} WORKTASK;

typedef struct _workresult {
	uint8_t cmd;
	uint8_t dev;
	uint8_t workid;
	uint8_t nonce[4];
} WORKRESULT;

typedef struct klondike_cfg {
	uint8_t cmd;
	uint8_t dev;
	uint8_t hashclock[2];
	uint8_t temptarget;
	uint8_t tempcritical;
	uint8_t fantarget;
	uint8_t pad2;
} WORKCFG;

typedef struct kline {
	union {
		HEADER hd;
		IDENTITY id;
		WORKSTATUS ws;
		WORKTASK wt;
		WORKRESULT wr;
		WORKCFG cfg;
	};
} KLINE;

#define zero_kline(_kline) memset((void *)(_kline), 0, sizeof(KLINE));

typedef struct device_info {
	uint32_t noncecount;
	uint32_t nextworkid;
	uint16_t lasthashcount;
	uint64_t totalhashcount;
	uint32_t rangesize;
	uint32_t *chipstats;
} DEVINFO;

typedef struct klist {
	struct klist *prev;
	struct klist *next;
	KLINE kline;
	struct timeval tv_when;
	int block_seq;
	bool ready;
	bool working;
} KLIST;

typedef struct jobque {
	int workqc;
	struct timeval last_update;
	bool overheat;
	bool flushed;
	int late_update_count;
	int late_update_sequential;
} JOBQUE;

struct klondike_info {
	pthread_rwlock_t stat_lock;
	struct thr_info replies_thr;
	cglock_t klist_lock;
	KLIST *used;
	KLIST *free;
	int kline_count;
	int used_count;
	int block_seq;
	KLIST *status;
	DEVINFO *devinfo;
	KLIST *cfg;
	JOBQUE *jobque;
	int noncecount;
	uint64_t hashcount;
	uint64_t errorcount;
	uint64_t noisecount;
	int incorrect_slave_sequential;

	// us Delay from USB reply to being processed
	double delay_count;
	double delay_total;
	double delay_min;
	double delay_max;

	struct timeval tv_last_nonce_received;

	// Time from recieving one nonce to the next
	double nonce_count;
	double nonce_total;
	double nonce_min;
	double nonce_max;

	int wque_size;
	int wque_cleared;

	bool initialised;
};

static KLIST *new_klist_set(struct cgpu_info *klncgpu)
{
	struct klondike_info *klninfo = (struct klondike_info *)(klncgpu->device_data);
	KLIST *klist = NULL;
	int i;

	klist = calloc(MAX_KLINES, sizeof(*klist));
	if (!klist)
		quit(1, "Failed to calloc klist - when old count=%d", klninfo->kline_count);

	klninfo->kline_count += MAX_KLINES;

	klist[0].prev = NULL;
	klist[0].next = &(klist[1]);
	for (i = 1; i < MAX_KLINES-1; i++) {
		klist[i].prev = &klist[i-1];
		klist[i].next = &klist[i+1];
	}
	klist[MAX_KLINES-1].prev = &(klist[MAX_KLINES-2]);
	klist[MAX_KLINES-1].next = NULL;

	return klist;
}

static KLIST *allocate_kitem(struct cgpu_info *klncgpu)
{
	struct klondike_info *klninfo = (struct klondike_info *)(klncgpu->device_data);
	KLIST *kitem = NULL;
	int ran_out = 0;
	char errbuf[1024];

	cg_wlock(&klninfo->klist_lock);

	if (klninfo->free == NULL) {
		ran_out = klninfo->kline_count;
		klninfo->free = new_klist_set(klncgpu);
		snprintf(errbuf, sizeof(errbuf),
				 "%s%i: KLINE count exceeded %d, now %d",
				 klncgpu->drv->name, klncgpu->device_id,
				 ran_out, klninfo->kline_count);
	}

	kitem = klninfo->free;

	klninfo->free = klninfo->free->next;
	if (klninfo->free)
		klninfo->free->prev = NULL;

	kitem->next = klninfo->used;
	kitem->prev = NULL;
	if (kitem->next)
		kitem->next->prev = kitem;
	klninfo->used = kitem;

	kitem->ready = false;
	kitem->working = false;

	memset((void *)&(kitem->kline), 0, sizeof(kitem->kline));

	klninfo->used_count++;

	cg_wunlock(&klninfo->klist_lock);

	if (ran_out > 0)
		applog(LOG_WARNING, "%s", errbuf);

	return kitem;
}

static KLIST *release_kitem(struct cgpu_info *klncgpu, KLIST *kitem)
{
	struct klondike_info *klninfo = (struct klondike_info *)(klncgpu->device_data);

	cg_wlock(&klninfo->klist_lock);

	if (kitem == klninfo->used)
		klninfo->used = kitem->next;

	if (kitem->next)
		kitem->next->prev = kitem->prev;
	if (kitem->prev)
		kitem->prev->next = kitem->next;

	kitem->next = klninfo->free;
	if (klninfo->free)
		klninfo->free->prev = kitem;

	kitem->prev = NULL;

	klninfo->free = kitem;

	klninfo->used_count--;

	cg_wunlock(&klninfo->klist_lock);

	return NULL;
}

static double cvtKlnToC(uint8_t temp)
{
	double Rt, stein, celsius;

	if (temp == 0)
		return 0.0;

	Rt = 1000.0 * 255.0 / (double)temp - 1000.0;

	stein = log(Rt / 2200.0) / 3987.0;

	stein += 1.0 / (double)(25.0 + 273.15);

	celsius = (1.0 / stein) - 273.15;

	// For display of bad data
	if (celsius < 0.0)
		celsius = 0.0;
	if (celsius > 200.0)
		celsius = 200.0;

	return celsius;
}

static int cvtCToKln(double deg)
{
	double Rt, stein, temp;

	if (deg < 0.0)
		deg = 0.0;

	stein = 1.0 / (deg + 273.15);

	stein -= 1.0 / (double)(25.0 + 273.15);

	Rt = exp(stein * 3987.0) * 2200.0;

	if (Rt == -1000.0)
		Rt++;

	temp = 1000.0 * 256.0 / (Rt + 1000.0);

	if (temp > 255)
		temp = 255;
	if (temp < 0)
		temp = 0;

	return (int)temp;
}

// Change this to LOG_WARNING if you wish to always see the replies
#define READ_DEBUG LOG_DEBUG

static void display_kline(struct cgpu_info *klncgpu, KLINE *kline, const char *msg)
{
	char *hexdata;

	switch (kline->hd.cmd) {
		case KLN_CMD_NONCE:
			applog(READ_DEBUG,
				"%s%i:%d %s work [%c] dev=%d workid=%d"
				" nonce=0x%08x",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->wr.dev), msg, kline->wr.cmd,
				(int)(kline->wr.dev),
				(int)(kline->wr.workid),
				(unsigned int)K_NONCE(kline->wr.nonce) - 0xC0);
			break;
		case KLN_CMD_STATUS:
		case KLN_CMD_WORK:
		case KLN_CMD_ENABLE:
		case KLN_CMD_ABORT:
			applog(READ_DEBUG,
				"%s%i:%d %s status [%c] dev=%d chips=%d"
				" slaves=%d workcq=%d workid=%d temp=%d fan=%d"
				" errors=%d hashes=%d max=%d noise=%d",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->ws.dev), msg, kline->ws.cmd,
				(int)(kline->ws.dev),
				(int)(kline->ws.chipcount),
				(int)(kline->ws.slavecount),
				(int)(kline->ws.workqc),
				(int)(kline->ws.workid),
				(int)(kline->ws.temp),
				(int)(kline->ws.fanspeed),
				(int)(kline->ws.errorcount),
				K_HASHCOUNT(kline->ws.hashcount),
				K_MAXCOUNT(kline->ws.maxcount),
				(int)(kline->ws.noise));
			break;
		case KLN_CMD_CONFIG:
			applog(READ_DEBUG,
				"%s%i:%d %s config [%c] dev=%d clock=%d"
				" temptarget=%d tempcrit=%d fan=%d",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->cfg.dev), msg, kline->cfg.cmd,
				(int)(kline->cfg.dev),
				K_HASHCLOCK(kline->cfg.hashclock),
				(int)(kline->cfg.temptarget),
				(int)(kline->cfg.tempcritical),
				(int)(kline->cfg.fantarget));
			break;
		case KLN_CMD_IDENT:
			applog(READ_DEBUG,
				"%s%i:%d %s info [%c] version=0x%02x prod=%.7s"
				" serial=0x%08x",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->hd.dev), msg, kline->hd.cmd,
				(int)(kline->id.version),
				kline->id.product,
				(unsigned int)K_SERIAL(kline->id.serial));
			break;
		default:
			hexdata = bin2hex((unsigned char *)&(kline->hd.dev), REPLY_SIZE - 1);
			applog(LOG_ERR,
				"%s%i:%d %s [%c:%s] unknown and ignored",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->hd.dev), msg, kline->hd.cmd,
				hexdata);
			free(hexdata);
			break;
	}
}

static void display_send_kline(struct cgpu_info *klncgpu, KLINE *kline, const char *msg)
{
	char *hexdata;

	switch (kline->hd.cmd) {
		case KLN_CMD_WORK:
			applog(READ_DEBUG,
				"%s%i:%d %s work [%c] dev=%d workid=0x%02x ...",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->wt.dev), msg, kline->ws.cmd,
				(int)(kline->wt.dev),
				(int)(kline->wt.workid));
			break;
		case KLN_CMD_CONFIG:
			applog(READ_DEBUG,
				"%s%i:%d %s config [%c] dev=%d clock=%d"
				" temptarget=%d tempcrit=%d fan=%d",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->cfg.dev), msg, kline->cfg.cmd,
				(int)(kline->cfg.dev),
				K_HASHCLOCK(kline->cfg.hashclock),
				(int)(kline->cfg.temptarget),
				(int)(kline->cfg.tempcritical),
				(int)(kline->cfg.fantarget));
			break;
		case KLN_CMD_IDENT:
		case KLN_CMD_STATUS:
		case KLN_CMD_ABORT:
			applog(READ_DEBUG,
				"%s%i:%d %s cmd [%c]",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->hd.dev), msg, kline->hd.cmd);
			break;
		case KLN_CMD_ENABLE:
			applog(READ_DEBUG,
				"%s%i:%d %s enable [%c] enable=%c",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->hd.dev), msg, kline->hd.cmd,
				(char)(kline->hd.buf[0]));
			break;
		case KLN_CMD_NONCE:
		default:
			hexdata = bin2hex((unsigned char *)&(kline->hd.dev), REPLY_SIZE - 1);
			applog(LOG_ERR,
				"%s%i:%d %s [%c:%s] unknown/unexpected and ignored",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->hd.dev), msg, kline->hd.cmd,
				hexdata);
			free(hexdata);
			break;
	}
}

static bool SendCmd(struct cgpu_info *klncgpu, KLINE *kline, int datalen)
{
	int err, amt, writ;

	if (klncgpu->usbinfo.nodev)
		return false;

	display_send_kline(klncgpu, kline, msg_send);
	writ = KSENDHD(datalen);
	err = usb_write(klncgpu, (char *)kline, writ, &amt, C_REQUESTRESULTS);
	if (err < 0 || amt != writ) {
		applog(LOG_ERR, "%s%i:%d Cmd:%c Dev:%d, write failed (%d:%d:%d)",
				klncgpu->drv->name, klncgpu->device_id,
				(int)(kline->hd.dev),
				kline->hd.cmd, (int)(kline->hd.dev),
				writ, amt, err);
		return false;
	}

	return true;
}

static KLIST *GetReply(struct cgpu_info *klncgpu, uint8_t cmd, uint8_t dev)
{
	struct klondike_info *klninfo = (struct klondike_info *)(klncgpu->device_data);
	KLIST *kitem;
	int retries = CMD_REPLY_RETRIES;

	while (retries-- > 0 && klncgpu->shutdown == false) {
		cgsleep_ms(REPLY_WAIT_TIME);
		cg_rlock(&klninfo->klist_lock);
		kitem = klninfo->used;
		while (kitem) {
			if (kitem->kline.hd.cmd == cmd &&
			    kitem->kline.hd.dev == dev &&
			    kitem->ready == true && kitem->working == false) {
				kitem->working = true;
				cg_runlock(&klninfo->klist_lock);
				return kitem;
			}
			kitem = kitem->next;
		}
		cg_runlock(&klninfo->klist_lock);
	}
	return NULL;
}

static KLIST *SendCmdGetReply(struct cgpu_info *klncgpu, KLINE *kline, int datalen)
{
	if (!SendCmd(klncgpu, kline, datalen))
		return NULL;

	return GetReply(klncgpu, kline->hd.cmd, kline->hd.dev);
}

static bool klondike_get_stats(struct cgpu_info *klncgpu)
{
	struct klondike_info *klninfo = (struct klondike_info *)(klncgpu->device_data);
	KLIST *kitem;
	KLINE kline;
	int slaves, dev;

	if (klncgpu->usbinfo.nodev || klninfo->status == NULL)
		return false;

	applog(LOG_DEBUG, "%s%i: getting status",
			klncgpu->drv->name, klncgpu->device_id);

	rd_lock(&(klninfo->stat_lock));
	slaves = klninfo->status[0].kline.ws.slavecount;
	rd_unlock(&(klninfo->stat_lock));

	// loop thru devices and get status for each
	for (dev = 0; dev <= slaves; dev++) {
		zero_kline(&kline);
		kline.hd.cmd = KLN_CMD_STATUS;
		kline.hd.dev = dev;
		kitem = SendCmdGetReply(klncgpu, &kline, 0);
		if (kitem != NULL) {
			wr_lock(&(klninfo->stat_lock));
			memcpy((void *)(&(klninfo->status[dev])),
				(void *)kitem,
				sizeof(klninfo->status[dev]));
			wr_unlock(&(klninfo->stat_lock));
			kitem = release_kitem(klncgpu, kitem);
		} else {
			applog(LOG_ERR, "%s%i:%d failed to update stats",
					klncgpu->drv->name, klncgpu->device_id, dev);
		}
	}
	return true;
}

// TODO: this only enables the master (no slaves)
static bool kln_enable(struct cgpu_info *klncgpu)
{
	KLIST *kitem;
	KLINE kline;
	int tries = 2;
	bool ok = false;

	zero_kline(&kline);
	kline.hd.cmd = KLN_CMD_ENABLE;
	kline.hd.dev = 0;
	kline.hd.buf[0] = KLN_CMD_ENABLE_ON;
	
	while (tries-- > 0) {
		kitem = SendCmdGetReply(klncgpu, &kline, 1);
		if (kitem) {
			kitem = release_kitem(klncgpu, kitem);
			ok = true;
			break;
		}
		cgsleep_ms(50);
	}

	if (ok)
		cgsleep_ms(50);

	return ok;
}

static void kln_disable(struct cgpu_info *klncgpu, int dev, bool all)
{
	KLINE kline;
	int i;

	zero_kline(&kline);
	kline.hd.cmd = KLN_CMD_ENABLE;
	kline.hd.buf[0] = KLN_CMD_ENABLE_OFF;
	for (i = (all ? 0 : dev); i <= dev; i++) {
		kline.hd.dev = i;
		SendCmd(klncgpu, &kline, KSENDHD(1));
	}
}

static bool klondike_init(struct cgpu_info *klncgpu)
{
	struct klondike_info *klninfo = (struct klondike_info *)(klncgpu->device_data);
	KLIST *kitem;
	KLINE kline;
	int slaves, dev;

	klninfo->initialised = false;

	zero_kline(&kline);
	kline.hd.cmd = KLN_CMD_STATUS;
	kline.hd.dev = 0;
	kitem = SendCmdGetReply(klncgpu, &kline, 0);
	if (kitem == NULL)
		return false;

	slaves = kitem->kline.ws.slavecount;
	if (klninfo->status == NULL) {
		applog(LOG_DEBUG, "%s%i: initializing data",
				klncgpu->drv->name, klncgpu->device_id);

		// alloc space for status, devinfo, cfg and jobque for master and slaves
		klninfo->status = calloc(slaves+1, sizeof(*(klninfo->status)));
		if (unlikely(!klninfo->status))
			quit(1, "Failed to calloc status array in klondke_get_stats");
		klnin