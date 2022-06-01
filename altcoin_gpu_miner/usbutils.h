/*
 * Copyright 2012-2013 Andrew Smith
 * Copyright 2013 Con Kolivas <kernel@kolivas.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#ifndef USBUTILS_H
#define USBUTILS_H

#include <libusb.h>

#include "util.h"

#define EPI(x) (LIBUSB_ENDPOINT_IN | (unsigned char)(x))
#define EPO(x) (LIBUSB_ENDPOINT_OUT | (unsigned char)(x))


// For 0x0403:0x6014/0x6001 FT232H (and possibly others?) - BFL, BAS, BLT, LLT, AVA
#define FTDI_TYPE_OUT (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT)
#define FTDI_TYPE_IN (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN)

#define FTDI_REQUEST_RESET ((uint8_t)0)
#define FTDI_REQUEST_MODEM ((uint8_t)1)
#define FTDI_REQUEST_FLOW ((uint8_t)2)
#define FTDI_REQUEST_BAUD ((uint8_t)3)
#define FTDI_REQUEST_DATA ((uint8_t)4)
#define FTDI_REQUEST_LATENCY ((uint8_t)9)

#define FTDI_VALUE_RESET 0
#define FTDI_VALUE_PURGE_RX 1
#define FTDI_VALUE_PURGE_TX 2
#define FTDI_VALUE_LATENCY 1

// Baud
#define FTDI_VALUE_BAUD_BFL 0xc068
#define FTDI_INDEX_BAUD_BFL 0x0200
#define FTDI_VALUE_BAUD_BAS FTDI_VALUE_BAUD_BFL
#define FTDI_INDEX_BAUD_BAS FTDI_INDEX_BAUD_BFL
// LLT = BLT (same code)
#define FTDI_VALUE_BAUD_BLT 0x001a
#define FTDI_INDEX_BAUD_BLT 0x0000
