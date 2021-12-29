/*
 * Copyright 2013 Con Kolivas <kernel@kolivas.org>
 * Copyright 2013 Andrew Smith
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#ifndef BFLSC_H
#define BFLSC_H
#define BLANK ""
#define LFSTR "<LF>"

/*
 * Firmware
 * DRV_V2 expects (beyond V1) the GetInfo to return the chip count
 * The queues are 40 instead of 20 and are *usually* consumed and filled
 * in bursts due to e.g. a 16 chip device doing 16 items at a time and
 * returning 16 results at a time
 * If the device has varying chip speeds, it will gradually break up the
 * burst of results as we progress
 */
enum driver_version {
	BFLSC_DRVUNDEF = 0,
	BFLSC_DRV1,
	BFLSC_DRV2
};

/*
 * With Firmware 1.0.0 and a result queue of 20 the Max is:
 * inprocess = 12
 * max count = 9
 * 64+1+24+1+1+(1+8)*8+1 per line = 164 * 20
 * OK = 3
 * Total: 3304
 *
 * With Firmware 1.2.* and a result queue of 40 but a limit of 15 replies:
 * inprocess = 12
 * max count = 9
 * 64+1+24+1+1+1+1+(1+8)*8+1 per line = 166 * 15
 * OK = 3
 * Total: 2514
 *
 */
#define BFLSC_BUFSIZ (0x1000)

// Should be big enough
#define BFLSC_APPLOGSIZ 8192

#define BFLSC_INFO_TIMEOUT 999

#define BFLSC_DI_FIRMWARE "FIRMWARE"
#define BFLSC_DI_ENGINES "ENGINES"
#define BFLSC_DI_JOBSINQUE "JOBS IN QUEUE"
#define BFLSC_DI_XLINKMODE "XLINK MODE"
#define BFLSC_DI_XLINKPRESENT "XLINK PRESENT"
#define BFLSC_DI_DEVICESINCHAIN "DEVICES IN CHAIN"
#define BFLSC_DI_CHAINPRESENCE "CHAIN PRESENCE MASK"
#define BFLSC_DI_CHIPS "CHIP PARALLELIZATION"
#define BFLSC_DI_CHIPS_PARALLEL "YES"

#define FULLNONCE 0x100000000ULL

struct bflsc_dev {
	// Work
	unsigned int ms_work;
	int work_queued;
	int work_complete;
	int nonces_hw; // TODO: this - need to add a paramter to submit_nonce()
			// so can pass 'dev' to hw_error
	uint64_t hashes_unsent;
	uint64_t hashes_sent;
	uint64_t nonces_found;

	struct timeval last_check_result;
	struct timeval last_dev_result; // array > 0
	struct timeval last_nonce_result; // > 0 nonce

	// Info
	char getinfo[(BFLSC_BUFSIZ+4)*4];
	char *firmware;
	int engines; // each engine represents a 'thread' in a chip
	char *xlink_mode;
	char *xlink_present;
	char *chips;

	// Status
	bool dead; // TODO: handle seperate x-link devices failing?
	bool overheat;

	// Stats
	float temp1;
	float temp2;
	float vcc1;
	float vcc2;
	float vmain;
	float temp1_max;
	float temp2_max;
	time_t temp1_max_time;
	time_t temp2_max_time;
	float temp1_5min_av; // TODO:
	float temp2_5min_av; // TODO:

	// To handle the fact that flushing the queue may not remove all work
	// (normally one item is still being processed)
	// and also that once the queue is flushed, results may still be in
	// the output queue - but we don't want to process them at the time of doing an LP
	// when result_id > flush_id+1, flushed work can be discarded since it
	// is no longer in the device
	uint64_t flush_id; // counter when results were last flushed
	uint64_t result_id; // counter when results were last checked
	bool flushed; // are any flushed?
};

#define QUE_MAX_RESULTS 8

struct bflsc_info {
	enum driver_version driver_version;
	pthread_rwlock_t stat_lock;
	struct thr_info results_thr;
	uint64_t hashes_sent;
	uint32_t update_count;
	struct timeval last_update;
	int sc_count;
	struct bflsc_dev *sc_devs;
	unsigned int scan_sleep_time;
	unsigned int results_sleep_time;
	unsigned int default_ms_work;
	bool shutdown;
	bool flash_led;
	bool not_first_work; // allow ignoring the first nonce error
	bool fanauto;
	int que_size;
	int que_full_enough;
	int que_watermark;
	int que_low;
	int que_noncecount;
	int que_fld_min;
	int que_fld_max;
	int flush_size;
	// count of given size, [+2] is for any > QUE_MAX_RESULTS
	uint64_t result_size[QUE_MAX_RESULTS+2];
};

#define BFLSC_XLINKHDR '@'
#define BFLSC_MAXPAYLOAD 255

struct DataForwardToChain {
	uint8_t header;
	uint8_t payloadSize;
	uint8_t deviceAddress;
	uint8_t payloadData[BFLSC_MAXPAYLOAD];
};

#define DATAFORWARDSIZE(data) (1 + 1 + 1 + data.payloadSize)

#define MIDSTATE_BYTES 32
#define MERKLE_OFFSET 64
#define MERKLE_BYTES 12
#define BFLSC_QJOBSIZ (MIDSTATE_BYTES+MERKLE_BYTES+1)
#define BFLSC_EOB 0xaa

struct QueueJobStructure {
	uint8_t payloadSize;
	uint8_t midState[MIDSTATE_BYTES];
	uint8_t blockData[MERKLE_BYTES];
	uint8_t endOfBlock;
};

#define QUE_RES_LINES_MIN 3
#define QUE_MIDSTATE 0
#define QUE_BLOCKDATA 1

#define QUE_NONCECOUNT_V1 2
#define QUE_FLD_MIN_V1 3
#define QUE_FLD_MAX_V1 (QUE_MAX_RESULTS+QUE_FLD_MIN_V1)

#define QUE_CHIP_V2 2
#define QUE_NONCECOUNT_V2 3
#define QUE_FLD_MIN_V2 4
#define QUE_FLD_MAX_V2 (QUE_MAX_RESULTS+QUE_FLD_MIN_V2)

#define BFLSC_SIGNATURE 0xc1
#define BFLSC_EOW 0xfe

// N.B. this will only work with 5 jobs
// requires a different jobs[N] for each job count
// but really only need to handle 5 anyway
struct QueueJobPackStructure {
	uint8_t payloadSize;
	uint8_t signature;
	uint8_t jobsInArray;
	struct QueueJobStructure jobs[5];
	uint8_t endOfWrapper;
};

// TODO: Implement in API and also in usb device selection
struct SaveString {
	uint8_t payloadSize;
	uint8_t payloadData[BFLSC_MAXPAYLOAD];
};

// Commands (Single Stage)
#define BFLSC_IDENTIFY "ZGX"
#define BFLSC_IDENTIFY_LEN (sizeof(BFLSC_IDENTIFY)-