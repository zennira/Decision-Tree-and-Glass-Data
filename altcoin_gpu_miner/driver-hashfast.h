/*
 * Copyright 2013 Con Kolivas <kernel@kolivas.org>
 * Copyright 2013 Hashfast
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#ifndef HASHFAST_H
#define HASHFAST_H

#ifdef USE_HASHFAST
#include "miner.h"
#include "elist.h"
#include "hf_protocol.h"

#define HASHFAST_MINER_THREADS 1

// Matching fields for hf_statistics, but large #s for local accumulation, per-die
struct hf_long_statistics {
	uint64_t rx_header_crc;                     // Header CRCs
	uint64_t rx_body_crc;                       // Data CRCs
	uint64_t rx_header_timeouts;                // Header timeouts
	uint64_t rx_body_timeouts;                  // Data timeouts
	uint64_t core_nonce_fifo_full;              // Core nonce Q overrun events
	uint64_t array_nonce_fifo_full;             // System nonce Q overrun events
	uint64_t stats_overrun;                     // Overrun in statistics reporting
};

// Matching fields for hf_usb_stats1, but large #s for local accumulation, per device
struct hf_long_usb_stats1 {
	// USB incoming
	uint64_t usb_rx_preambles;
	uint64_t usb_rx_receive_byte_errors;
	uint64_t usb_rx_bad_hcrc;

	// USB outgoing
	uint64_t usb_tx_attempts;
	uint64_t usb_tx_packets;
	uint64_t usb_tx_timeouts;
	uint64_t usb_tx_incompletes;
	uint64_t usb_tx_endpointstalled;
	uint64_t usb_tx_disconnected;
	uint64_t usb_tx_suspended;
#if 0
	/* We don't care about UART stats */
	// UART transmit
	uint64_t uart_tx_queue_dma;
	uint64_t uart_tx_interrupts;

	// UART receive
	uint64_t uart_rx_preamble_ints;
	uint64_t uart_rx_missed_preamble_ints;
	uint64_t uart_rx_header_done;
	uint64_t uart_rx_data_done;
	uint64_t uart_rx_bad_hcrc;
	uint64_t uart_rx_bad_dma;
	uint64_t uart_rx_short_dma;
	uint64_t uart_rx_buffers_full;
#endif

	uint8_t  max_tx_buffers;
	uint8_t  max_rx_buffers;
};

struct hashfast_info {
	int asic_count;                             // # of chips in the chain
	int core_count;                             // # of cores per chip
	int device_type;                            // What sort of device this is
	int num_sequence;                           // A power of 2. What the sequence num