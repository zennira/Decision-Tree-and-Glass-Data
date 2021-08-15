/*
 * Public libusb header file
 * Copyright (C) 2007-2008 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>
 * Copyright (C) 2012-2013 Nathan Hjelm <hjelmn@cs.unm.edu>
 * Copyright (C) 2012 Peter Stuge <peter@stuge.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBUSB_H
#define LIBUSB_H

#ifdef _MSC_VER
/* on MS environments, the inline keyword is available in C++ only */
#define inline __inline
/* ssize_t is also not available (copy/paste from MinGW) */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#undef ssize_t
#ifdef _WIN64
  typedef __int64 ssize_t;
#else
  typedef int ssize_t;
#endif /* _WIN64 */
#endif /* _SSIZE_T_DEFINED */
#endif /* _MSC_VER */

/* stdint.h is also not usually available on MS */
#if defined(_MSC_VER) && (_MSC_VER < 1600) && (!defined(_STDINT)) && (!defined(_STDINT_H))
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#else
#include <stdint.h>
#endif

#include <sys/types.h>
#include <time.h>
#include <limits.h>

#if defined(__linux) || defined(__APPLE__) || defined(__CYGWIN__)
#include <sys/time.h>
#endif

/* 'interface' might be defined as a macro on Windows, so we need to
 * undefine it so as not to break the current libusb API, because
 * libusb_config_descriptor has an 'interface' member
 * As this can be problematic if you include windows.h after libusb.h
 * in your sources, we force windows.h to be included first. */
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#if defined(interface)
#undef interface
#endif
#endif

/** \def LIBUSB_CALL
 * \ingroup misc
 * libusb's Windows calling convention.
 *
 * Under Windows, the selection of available compilers and configurations
 * means that, unlike other platforms, there is not <em>one true calling
 * convention</em> (calling convention: the manner in which parameters are
 * passed to funcions in the generated assembly code).
 *
 * Matching the Windows API itself, libusb uses the WINAPI convention (which
 * translates to the <tt>stdcall</tt> convention) and guarantees that the
 * library is compiled in this way. The public header file also includes
 * appropriate annotations so that your own software will use the right
 * convention, even if another convention is being used by default within
 * your codebase.
 *
 * The one consideration that you must apply in your software is to mark
 * all functions which you use as libusb callbacks with this LIBUSB_CALL
 * annotation, so that they too get compiled for the correct calling
 * convention.
 *
 * On non-Windows operating systems, this macro is defined as nothing. This
 * means that you can apply it to your code without worrying about
 * cross-platform compatibility.
 */
/* LIBUSB_CALL must be defined on both definition and declaration of libusb
 * functions. You'd think that declaration would be enough, but cygwin will
 * complain about conflicting types unless both are marked this way.
 * The placement of this macro is important too; it must appear after the
 * return type, before the function name. See internal documentation for
 * API_EXPORTED.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#define LIBUSB_CALL WINAPI
#else
#define LIBUSB_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** \def libusb_cpu_to_le16
 * \ingroup misc
 * Convert a 16-bit value from host-endian to little-endian format. On
 * little endian systems, this function does nothing. On big endian systems,
 * the bytes are swapped.
 * \param x the host-endian value to convert
 * \returns the value in little-endian byte order
 */
static inline uint16_t libusb_cpu_to_le16(const uint16_t x)
{
	union {
		uint8_t  b8[2];
		uint16_t b16;
	} _tmp;
	_tmp.b8[1] = x >> 8;
	_tmp.b8[0] = x & 0xff;
	return _tmp.b16;
}

/** \def libusb_le16_to_cpu
 * \ingroup misc
 * Convert a 16-bit value from little-endian to host-endian format. On
 * little endian systems, this function does nothing. On big endian systems,
 * the bytes are swapped.
 * \param x the little-endian value to convert
 * \returns the value in host-endian byte order
 */
#define libusb_le16_to_cpu libusb_cpu_to_le16

/* standard USB stuff */

/** \ingroup desc
 * Device and/or Interface Class codes */
enum libusb_class_code {
	/** In the context of a \ref libusb_device_descriptor "device descriptor",
	 * this bDeviceClass value indicates that each interface specifies its
	 * own class information and all interfaces operate independently.
	 */
	LIBUSB_CLASS_PER_INTERFACE = 0,

	/** Audio class */
	LIBUSB_CLASS_AUDIO = 1,

	/** Communications class */
	LIBUSB_CLASS_COMM = 2,

	/** Human Interface Device class */
	LIBUSB_CLASS_HID = 3,

	/** Physical */
	LIBUSB_CLASS_PHYSICAL = 5,

	/** Printer class */
	LIBUSB_CLASS_PRINTER = 7,

	/** Image class */
	LIBUSB_CLASS_PTP = 6, /* legacy name from libusb-0.1 usb.h */
	LIBUSB_CLASS_IMAGE = 6,

	/** Mass storage class */
	LIBUSB_CLASS_MASS_STORAGE = 8,

	/** Hub class */
	LIBUSB_CLASS_HUB = 9,

	/** Data class */
	LIBUSB_CLASS_DATA = 10,

	/** Smart Card */
	LIBUSB_CLASS_SMART_CARD = 0x0b,

	/** Content Security */
	LIBUSB_CLASS_CONTENT_SECURITY = 0x0d,

	/** Video */
	LIBUSB_CLASS_VIDEO = 0x0e,

	/** Personal Healthcare */
	LIBUSB_CLASS_PERSONAL_HEALTHCARE = 0x0f,

	/** Diagnostic Device */
	LIBUSB_CLASS_DIAGNOSTIC_DEVICE = 0xdc,

	/** Wireless class */
	LIBUSB_CLASS_WIRELESS = 0xe0,

	/** Application class */
	LIBUSB_CLASS_APPLICATION = 0xfe,

	/** Class is vendor-specific */
	LIBUSB_CLASS_VENDOR_SPEC = 0xff
};

/** \ingroup desc
 * Descriptor types as defined by the USB specification. */
enum libusb_descriptor_type {
	/** Device descriptor. See libusb_device_descriptor. */
	LIBUSB_DT_DEVICE = 0x01,

	/** Configuration descriptor. See libusb_config_descriptor. */
	LIBUSB_DT_CONFIG = 0x02,

	/** String descriptor */
	LIBUSB_DT_STRING = 0x03,

	/** Interface descriptor. See libusb_interface_descriptor. */
	LIBUSB_DT_INTERFACE = 0x04,

	/** Endpoint descriptor. See libusb_endpoint_descriptor. */
	LIBUSB_DT_ENDPOINT = 0x05,

	/** HID descriptor */
	LIBUSB_DT_HID = 0x21,

	/** HID report descriptor */
	LIBUSB_DT_REPORT = 0x22,

	/** Physical descriptor */
	LIBUSB_DT_PHYSICAL = 0x23,

	/** Hub descriptor */
	LIBUSB_DT_HUB = 0x29,

	/** BOS descriptor */
	LIBUSB_DT_BOS = 0x0f,

	/** Device Capability descriptor */
	LIBUSB_DT_DEVICE_CAPABILITY = 0x10,

	/** SuperSpeed Endpoint Companion descriptor */
	LIBUSB_DT_SS_ENDPOINT_COMPANION = 0x30
};

/* Descriptor sizes per descriptor type */
#define LIBUSB_DT_DEVICE_SIZE			18
#define LIBUSB_DT_CONFIG_SIZE			9
#define LIBUSB_DT_INTERFACE_SIZE		9
#define LIBUSB_DT_ENDPOINT_SIZE		7
#define LIBUSB_DT_ENDPOINT_AUDIO_SIZE	9	/* Audio extension */
#define LIBUSB_DT_HUB_NONVAR_SIZE		7
#define LIBUSB_DT_SS_ENDPOINT_COMPANION_SIZE    6
#define LIBUSB_DT_BOS_SIZE			5
#define LIBUSB_USB_2_0_EXTENSION_DEVICE_CAPABILITY_SIZE	7
#define LIBUSB_SS_USB_DEVICE_CAPABILITY_SIZE	10
#define LIBUSB_DT_BOS_MAX_SIZE		((LIBUSB_DT_BOS_SIZE) + \
					(LIBUSB_USB_2_0_EXTENSION_DEVICE_CAPABILITY_SIZE) + \
					(LIBUSB_SS_USB_DEVICE_CAPABILITY_SIZE))

#define LIBUSB_ENDPOINT_ADDRESS_MASK	0x0f    /* in bEndpointAddress */
#define LIBUSB_ENDPOINT_DIR_MASK	