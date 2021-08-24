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
#define LIBUSB_ENDPOINT_DIR_MASK		0x80

/** \ingroup desc
 * Endpoint direction. Values for bit 7 of the
 * \ref libusb_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 */
enum libusb_endpoint_direction {
	/** In: device-to-host */
	LIBUSB_ENDPOINT_IN = 0x80,

	/** Out: host-to-device */
	LIBUSB_ENDPOINT_OUT = 0x00
};

#define LIBUSB_TRANSFER_TYPE_MASK			0x03    /* in bmAttributes */

/** \ingroup desc
 * Endpoint transfer type. Values for bits 0:1 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "endpoint attributes" field.
 */
enum libusb_transfer_type {
	/** Control endpoint */
	LIBUSB_TRANSFER_TYPE_CONTROL = 0,

	/** Isochronous endpoint */
	LIBUSB_TRANSFER_TYPE_ISOCHRONOUS = 1,

	/** Bulk endpoint */
	LIBUSB_TRANSFER_TYPE_BULK = 2,

	/** Interrupt endpoint */
	LIBUSB_TRANSFER_TYPE_INTERRUPT = 3
};

/** \ingroup misc
 * Standard requests, as defined in table 9-3 of the USB2 specifications */
enum libusb_standard_request {
	/** Request status of the specific recipient */
	LIBUSB_REQUEST_GET_STATUS = 0x00,

	/** Clear or disable a specific feature */
	LIBUSB_REQUEST_CLEAR_FEATURE = 0x01,

	/* 0x02 is reserved */

	/** Set or enable a specific feature */
	LIBUSB_REQUEST_SET_FEATURE = 0x03,

	/* 0x04 is reserved */

	/** Set device address for all future accesses */
	LIBUSB_REQUEST_SET_ADDRESS = 0x05,

	/** Get the specified descriptor */
	LIBUSB_REQUEST_GET_DESCRIPTOR = 0x06,

	/** Used to update existing descriptors or add new descriptors */
	LIBUSB_REQUEST_SET_DESCRIPTOR = 0x07,

	/** Get the current device configuration value */
	LIBUSB_REQUEST_GET_CONFIGURATION = 0x08,

	/** Set device configuration */
	LIBUSB_REQUEST_SET_CONFIGURATION = 0x09,

	/** Return the selected alternate setting for the specified interface */
	LIBUSB_REQUEST_GET_INTERFACE = 0x0A,

	/** Select an alternate interface for the specified interface */
	LIBUSB_REQUEST_SET_INTERFACE = 0x0B,

	/** Set then report an endpoint's synchronization frame */
	LIBUSB_REQUEST_SYNCH_FRAME = 0x0C,
};

/** \ingroup misc
 * Request type bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. */
enum libusb_request_type {
	/** Standard */
	LIBUSB_REQUEST_TYPE_STANDARD = (0x00 << 5),

	/** Class */
	LIBUSB_REQUEST_TYPE_CLASS = (0x01 << 5),

	/** Vendor */
	LIBUSB_REQUEST_TYPE_VENDOR = (0x02 << 5),

	/** Reserved */
	LIBUSB_REQUEST_TYPE_RESERVED = (0x03 << 5)
};

/** \ingroup misc
 * Recipient bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. Values 4 through 31 are reserved. */
enum libusb_request_recipient {
	/** Device */
	LIBUSB_RECIPIENT_DEVICE = 0x00,

	/** Interface */
	LIBUSB_RECIPIENT_INTERFACE = 0x01,

	/** Endpoint */
	LIBUSB_RECIPIENT_ENDPOINT = 0x02,

	/** Other */
	LIBUSB_RECIPIENT_OTHER = 0x03,
};

#define LIBUSB_ISO_SYNC_TYPE_MASK		0x0C

/** \ingroup desc
 * Synchronization type for isochronous endpoints. Values for bits 2:3 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 */
enum libusb_iso_sync_type {
	/** No synchronization */
	LIBUSB_ISO_SYNC_TYPE_NONE = 0,

	/** Asynchronous */
	LIBUSB_ISO_SYNC_TYPE_ASYNC = 1,

	/** Adaptive */
	LIBUSB_ISO_SYNC_TYPE_ADAPTIVE = 2,

	/** Synchronous */
	LIBUSB_ISO_SYNC_TYPE_SYNC = 3
};

#define LIBUSB_ISO_USAGE_TYPE_MASK 0x30

/** \ingroup desc
 * Usage type for isochronous endpoints. Values for bits 4:5 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 */
enum libusb_iso_usage_type {
	/** Data endpoint */
	LIBUSB_ISO_USAGE_TYPE_DATA = 0,

	/** Feedback endpoint */
	LIBUSB_ISO_USAGE_TYPE_FEEDBACK = 1,

	/** Implicit feedback Data endpoint */
	LIBUSB_ISO_USAGE_TYPE_IMPLICIT = 2,
};

/** \ingroup desc
 * A structure representing the standard USB device descriptor. This
 * descriptor is documented in section 9.6.1 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_device_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE LIBUSB_DT_DEVICE in this
	 * context. */
	uint8_t  bDescriptorType;

	/** USB specification release number in binary-coded decimal. A value of
	 * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc. */
	uint16_t bcdUSB;

	/** USB-IF class code for the device. See \ref libusb_class_code. */
	uint8_t  bDeviceClass;

	/** USB-IF subclass code for the device, qualified by the bDeviceClass
	 * value */
	uint8_t  bDeviceSubClass;

	/** USB-IF protocol code for the device, qualified by the bDeviceClass and
	 * bDeviceSubClass values */
	uint8_t  bDeviceProtocol;

	/** Maximum packet size for endpoint 0 */
	uint8_t  bMaxPacketSize0;

	/** USB-IF vendor ID */
	uint16_t idVendor;

	/** USB-IF product ID */
	uint16_t idProduct;

	/** Device release number in binary-coded decimal */
	uint16_t bcdDevice;

	/** Index of string descriptor describing manufacturer */
	uint8_t  iManufacturer;

	/** Index of string descriptor describing product */
	uint8_t  iProduct;

	/** Index of string descriptor containing device serial number */
	uint8_t  iSerialNumber;

	/** Number of possible configurations */
	uint8_t  bNumConfigurations;
};

/** \ingroup desc
 * A structure representing the superspeed endpoint companion
 * descriptor. This descriptor is documented in section 9.6.7 of
 * the USB 3.0 specification. All ultiple-byte fields are represented in
 * host-endian format.
 */
struct libusb_ss_endpoint_companion_descriptor {

	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_SS_ENDPOINT_COMPANION in
	 * this context. */
	uint8_t  bDescriptorType;


	/** The maximum number of packets the endpoint can send or
	 *  recieve as part of a burst. */
	uint8_t  bMaxBurst;

	/** In bulk EP:	bits 4:0 represents the	maximum	number of
	 *  streams the	EP supports. In	isochronous EP:	bits 1:0
	 *  represents the Mult	- a zero based value that determines
	 *  the	maximum	number of packets within a service interval  */
	uint8_t  bmAttributes;

	/** The	total number of bytes this EP will transfer every
	 *  service interval. valid only for periodic EPs. */
	uint16_t wBytesPerInterval;
};

/** \ingroup desc
 * A structure representing the standard USB endpoint descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_endpoint_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_ENDPOINT LIBUSB_DT_ENDPOINT in
	 * this context. */
	uint8_t  bDescriptorType;

	/** The address of the endpoint described by this descriptor. Bits 0:3 are
	 * the endpoint number. Bits 4:6 are reserved. Bit 7 indicates direction,
	 * see \ref libusb_endpoint_direction.
	 */
	uint8_t  bEndpointAddress;

	/** Attributes which apply to the endpoint when it is configured using
	 * the bConfigurationValue. Bits 0:1 determine the transfer type and
	 * correspond to \ref libusb_transfer_type. Bits 2:3 are only used for
	 * isochronous endpoints and correspond to \ref libusb_iso_sync_type.
	 * Bits 4:5 are also only used for isochronous endpoints and correspond to
	 * \ref libusb_iso_usage_type. Bits 6:7 are reserved.
	 */
	uint8_t  bmAttributes;

	/** Maximum packet size this endpoint is capable of sending/receiving. */
	uint16_t wMaxPacketSize;

	/** Interval for polling endpoint for data transfers. */
	uint8_t  bInterval;

	/** For audio devices only: the rate at which synchronization feedback
	 * is provided. */
	uint8_t  bRefresh;

	/** For audio devices only: the address if the synch endpoint */
	uint8_t  bSynchAddress;

	/** Extra descriptors. If libusb encounters unknown endpoint descriptors,
	 * it will store them here, should you wish to parse them. */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes. */
	int extra_length;
};


/** \ingroup desc
 * A structure representing the standard USB interface descriptor. This
 * descriptor is documented in section 9.6.5 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_interface_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_INTERFACE LIBUSB_DT_INTERFACE
	 * in this context. */
	uint8_t  bDescriptorType;

	/** Number of this interface */
	uint8_t  bInterfaceNumber;

	/** Value used to select this alternate setting for this interface */
	uint8_t  bAlternateSetting;

	/** Number of endpoints used by this interface (excluding the control
	 * endpoint). */
	uint8_t  bNumEndpoints;

	/** USB-IF class code for this interface. See \ref libusb_class_code. */
	uint8_t  bInterfaceClass;

	/** USB-IF subclass code for this interface, qualified by the
	 * bInterfaceClass value */
	uint8_t  bInterfaceSubClass;

	/** USB-IF protocol code for this interface, qualified by the
	 * bInterfaceClass and bInterfaceSubClass values */
	uint8_t  bInterfaceProtocol;

	/** Index of string descriptor describing this interface */
	uint8_t  iInterface;

	/** Array of endpoint descriptors. This length of this array is determined
	 * by the bNumEndpoints field. */
	const struct libusb_endpoint_descriptor *endpoint;

	/** Extra descriptors. If libusb encounters unknown interface descriptors,
	 * it will store them here, should you wish to parse them. */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes. */
	int extra_length;
};

/** \ingroup desc
 * A collection of alternate settings for a particular USB interface.
 */
struct libusb_interface {
	/** Array of interface descriptors. The length of this array is determined
	 * by the num_altsetting field. */
	const struct libusb_interface_descriptor *altsetting;

	/** The number of alternate settings that belong to this interface */
	int num_altsetting;
};

/** \ingroup desc
 * A structure representing the standard USB configuration descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_config_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_CONFIG LIBUSB_DT_CONFIG
	 * in this context. */
	uint8_t  bDescriptorType;

	/** Total length of data returned for this configuration */
	uint16_t wTotalLength;

	/** Number of interfaces supported by this configuration */
	uint8_t  bNumInterfaces;

	/** Identifier value for this configuration */
	uint8_t  bConfigurationValue;

	/** Index of string descriptor describing this configuration */
	uint8_t  iConfiguration;

	/** Configuration characteristics */
	uint8_t  bmAttributes;

	/** Maximum power consumption of the USB device from this bus in this
	 * configuration when the device is fully opreation. Expressed in units
	 * of 2 mA. */
	uint8_t  MaxPower;

	/** Array of interfaces supported by this configuration. The length of
	 * this array is determined by the bNumInterfaces field. */
	const struct libusb_interface *interface;

	/** Extra descriptors. If libusb encounters unknown configuration
	 * descriptors, it will store them here, should you wish to parse them. */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes. */
	int extra_length;
};

/** \ingroup desc
 * A structure representing the BOS descriptor. This
 * descriptor is documented in section 9.6.2 of the USB 3.0
 * specification. All multiple-byte fields are represented in
 * host-endian format.
 */
struct libusb_bos_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_BOS LIBUSB_DT_BOS
	 * in this context. */
	uint8_t  bDescriptorType;

	/** Length of this descriptor and all of its sub descriptors */
	uint16_t wTotalLength;

	/** The number of separate device capability descriptors in
	 * the BOS */
	uint8_t  bNumDeviceCaps;

	/** USB 2.0 extension capability descriptor */
	struct libusb_usb_2_0_device_capability_descriptor *usb_2_0_ext_cap;

	/** SuperSpeed capabilty descriptor */
	struct libusb_ss_usb_device_capability_descriptor *ss_usb_cap;
};

/** \ingroup desc
 * A structure representing the device capability descriptor for
 * USB 2.0. This descriptor is documented in section 9.6.2.1 of
 * the USB 3.0 specification. All mutiple-byte fields are represented
 * in host-endian format.
 */
struct libusb_usb_2_0_device_capability_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_USB_CAP_TYPE_EXT
	 * LIBUSB_USB_CAP_TYPE_EXT in this context. */
	uint8_t  bDevCapabilityType;

	/** Bitmap encoding of supported device level features.
	 * A value of one in a bit location indicates a feature is
	 * supported; a value of zero indicates it is not supported.
	 * See \ref libusb_capability_attributes. */
	uint32_t  bmAttributes;
};

/** \ingroup desc
 * A structure representing the device capability descriptor for
 * USB 3.0. This descriptor is documented in section 9.6.2.2 of
 * the USB 3.0 specification. All mutiple-byte fields are represented
 * in host-endian format.
 */
struct libusb_ss_usb_device_capability_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_SS_USB_CAP_TYPE
	 * LIBUSB_SS_USB_CAP_TYPE in this context. */
	uint8_t  bDevCapabilityType;

	/** Bitmap encoding of supported device level features.
	 * A value of one in a bit location indicates a feature is
	 * supported; a value of zero indicates it is not supported.
	 * See \ref libusb_capability_attributes. */
	uint8_t  bmAttributes;

	/** Bitmap encoding of the speed supported by this device when
	 * operating in SuperSpeed mode. See \ref libusb_supported_speed. */
	uint16_t wSpeedSupported;

	/** The lowest speed at which all the functionality supported
	 * by the device is available to the user. For example if the
	 * device supports all its functionality when connected at
	 * full speed and above then it sets this value to 1. */
	uint8_t  bFunctionalitySupport;

	/** U1 Device Exit Latency. */
	uint8_t  bU1DevExitLat;

	/** U2 Device Exit Latency. */
	uint16_t bU2DevExitLat;
};


/** \ingroup asyncio
 * Setup packet for control transfers. */
struct libusb_control_setup {
	/** Request type. Bits 0:4 determine recipient, see
	 * \ref libusb_request_recipient. Bits 5:6 determine type, see
	 * \ref libusb_request_type. Bit 7 determines data transfer direction, see
	 * \ref libusb_endpoint_direction.
	 */
	uint8_t  bmRequestType;

	/** Request. If the type bits of bmRequestType are equal to
	 * \ref libusb_request_type::LIBUSB_REQUEST_TYPE_STANDARD
	 * "LIBUSB_REQUEST_TYPE_STANDARD" then this field refers to
	 * \ref libusb_standard_request. For other cases, use of this field is
	 * application-specific. */
	uint8_t  bRequest;

	/** Value. Varies according to request */
	uint16_t wValue;

	/** Index. Varies according to request, typically used to pass an index
	 * or offset */
	uint16_t wIndex;

	/** Number of bytes to transfer */
	uint16_t wLength;
};

#define LIBUSB_CONTROL_SETUP_SIZE (sizeof(struct libusb_control_setup))

/* libusb */

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;
struct libusb_hotplug_callback;

/** \ingroup lib
 * Structure representing the libusb version.
 */
struct libusb_version {
	/** Library major version. */
	const uint16_t major;

	/** Library minor version. */
	const uint16_t minor;

	/** Library micro version. */
	const uint16_t micro;

	/** Library nano version. This field is only nonzero on Windows. */
	const uint16_t nano;

	/** Library release candidate suffix string, e.g. "-rc4". */
	const char *rc;

	/** Output of `git describe --tags` at library build time. */
	const char *describe;
};

/** \ingroup lib
 * Structure representing a libusb session. The concept of individual libusb
 * sessions allows for your program to use two libraries (or dynamically
 * load two modules) which both independently use libusb. This will prevent
 * interference between the individual libusb users - for example
 * libusb_set_debug() will not affect the other user of the library, and
 * libusb_exit() will not destroy resources that the other user is still
 * using.
 *
 * Sessions are created by libusb_init() and destroyed through libusb_exit().
 * If your application is guaranteed to only ever include a single libusb
 * user (i.e. you), you do not have to worry about contexts: pass NULL in
 * every function call where a context is required. The default context
 * will be used.
 *
 * For more information, see \ref contexts.
 */
typedef struct libusb_context libusb_context;

/** \ingroup dev
 * Structure representing a USB device detected on the system. This is an
 * opaque type for which you are only ever provided with a pointer, usually
 * originating from libusb_get_device_list().
 *
 * Certain operations can be performed on a device, but in order to do any
 * I/O you will have to first obtain a device handle using libusb_open().
 *
 * Devices are reference counted with libusb_ref_device() and
 * libusb_unref_device(), and are freed when the reference count reaches 0.
 * New devices presented by libusb_get_device_list() have a reference count of
 * 1, and libusb_free_device_list() can optionally decrease the reference count
 * on all devices in the list. libusb_open() adds another reference which is
 * later destroyed by libusb_close().
 */
typedef struct libusb_device libusb_device;


/** \ingroup dev
 * Structure representing a handle on a USB device. This is an opaque type for
 * which you are only ever provided with a pointer, usually originating from
 * libusb_open().
 *
 * A device handle is used to perform I/O and other operations. When finished
 * with a device handle, you should call libusb_close().
 */
typedef struct libusb_device_handle libusb_device_handle;

/** \ingroup dev
 * Speed codes. Indicates the speed at which the device is operating.
 */
enum libusb_speed {
    /** The OS doesn't report or know the device speed. */
    LIBUSB_SPEED_UNKNOWN = 0,

    /** The device is operating at low speed (1.5MBit/s). */
    LIBUSB_SPEED_LOW = 1,

    /** The device is operating at full speed (12MBit/s). */
    LIBUSB_SPEED_FULL = 2,

    /** The device is operating at high speed (480MBit/s). */
    LIBUSB_SPEED_HIGH = 3,

    /** The device is operating at super speed (5000MBit/s). */
    LIBUSB_SPEED_SUPER = 4,
};

/** \ingroup dev
 * Supported speeds (wSpeedSupported) bitfield. Indicates what
 * speeds the device supports.
 */
enum libusb_supported_speed {
	/** Low speed operation supported (1.5MBit/s). */
	LIBUSB_LOW_SPEED_OPERATION  = 1,

	/** Full speed operation supported (12MBit/s). */
	LIBUSB_FULL_SPEED_OPERATION = 2,

	/** High speed operation supported (480MBit/s). */
	LIBUSB_HIGH_SPEED_OPERATION = 4,

	/** Superspeed operation supported (5000MBit/s). */
	LIBUSB_5GBPS_OPERATION      = 8,
};

/** \ingroup dev
 * Capability attributes
 */
enum libusb_capability_attributes {
	/** Supports Link Power Management (LPM) */
	LIBUSB_LPM_SUPPORT = 2,
};

/** \ingroup dev
 * USB capability types
 */
enum libusb_capability_type {
	/** USB 2.0 extension capability type */
	LIBUSB_USB_CAP_TYPE_EXT = 2,

	/** SuperSpeed capability type */
	LIBUSB_SS_USB_CAP_TYPE  = 3,
};

/** \ingroup misc
 * Error codes. Most libusb functions return 0 on success or one of these
 * codes on failure.
 * You can call \ref libusb_error_name() to retrieve a string representation
 * of an error code or \ret libusb_strerror() to get an english description
 * of an error code.
 */
enum libusb_error {
	/** Success (no error) */
	LIBUSB_SUCCESS = 0,

	/** Input/output error */
	LIBUSB_ERROR_IO = -1,

	/** Invalid parameter */
	LIBUSB_ERROR_INVALID_PARAM = -2,

	/** Access denied (insufficient permissions) */
	LIBUSB_ERROR_ACCESS = -3,

	/** No such device (it may have been disconnected) */
	LIBUSB_ERROR_NO_DEVICE = -4,

	/** Entity not found */
	LIBUSB_ERROR_NOT_FOUND = -5,

	/** Resource busy */
	LIBUSB_ERROR_BUSY = -6,

	/** Operation timed out */
	LIBUSB_ERROR_TIMEOUT = -7,

	/** Overflow */
	LIBUSB_ERROR_OVERFLOW = -8,

	/** Pipe error */
	LIBUSB_ERROR_PIPE = -9,

	/** System call interrupted (perhaps due to signal) */
	LIBUSB_ERROR_INTERRUPTED = -10,

	/** Insufficient memory */
	LIBUSB_ERROR_NO_MEM = -11,

	/** Operation not supported or unimplemented on this platform */
	LIBUSB_ERROR_NOT_SUPPORTED = -12,

	/* NB! Remember to update libusb_error_name() and
	   libusb_strerror() when adding new error codes here. */

	/** Other error */
	LIBUSB_ERROR_OTHER = -99,
};

/** \ingroup asyncio
 * Transfer status codes */
enum libusb_transfer_status {
	/** Transfer completed without error. Note that this does not indicate
	 * that the entire amount of requested data was transferred. */
	LIBUSB_TRANSFER_COMPLETED,

	/** Transfer failed */
	LIBUSB_TRANSFER_ERROR,

	/** Transfer timed out */
	LIBUSB_TRANSFER_TIMED_OUT,

	/** Transfer was cancelled */
	LIBUSB_TRANSFER_CANCELLED,

	/** For bulk/interrupt endpoints: halt condition detected (endpoint
	 * stalled). For control endpoints: control request not supported. */
	LIBUSB_TRANSFER_STALL,

	/** Device was disconnected */
	LIBUSB_TRANSFER_NO_DEVICE,

	/** Device sent more data than requested */
	LIBUSB_TRANSFER_OVERFLOW,
};

/** \ingroup asyncio
 * libusb_transfer.flags values */
enum libusb_transfer_flags {
	/** Report short frames as errors */
	LIBUSB_TRANSFER_SHORT_NOT_OK = 1<<0,

	/** Automatically free() transfer buffer during libusb_free_transfer() */
	LIBUSB_TRANSFER_FREE_BUFFER = 1<<1,

	/** Automatically call libusb_free_transfer() after callback returns.
	 * If this flag is set, it is illegal to call libusb_free_transfer()
	 * from your transfer callback, as this will result in a double-free
	 * when this flag is acted upon. */
	LIBUSB_TRANSFER_FREE_TRANSFER = 1<<2,

	/** Terminate transfers that are a multiple of the endpoint's
	 * wMaxPacketSize with an extra zero length packet. This is useful
	 * when a device protocol mandates that each logical request is
	 * terminated by an incomplete packet (i.e. the logical requests are
	 * not separated by other means).
	 *
	 * This flag only affects host-to-device transfers to bulk and interrupt
	 * endpoints. In other situations, it is ignored.
	 *
	 * This flag only affects transfers with a length that is a multiple of
	 * the endpoint's wMaxPacketSize. On transfers of other lengths, this
	 * flag has no effect. Therefore, if you are working with a device that
	 * needs a ZLP whenever the end of the logical request falls on a packet
	 * boundary, then it is sensible to set this flag on <em>every</em>
	 * transfer (you do not have to worry about only setting it on transfers
	 * that end on the boundary).
	 *
	 * This flag is currently only supported on Linux.
	 * On other systems, libusb_submit_transfer() will return
	 * LIBUSB_ERROR_NOT_SUPPORTED for every transfer where this flag is set.
	 *
	 * Available since libusb-1.0.9.
	 */
	LIBUSB_TRANSFER_ADD_ZERO_PACKET = 1 << 3,
};

/** \ingroup asyncio
 * Isochronous packet descriptor. */
struct libusb_iso_packet_descriptor {
	/** Length of data to request in this packet */
	unsigned int length;

	/** Amount of data that was actually transferred */
	unsigned int actual_length;

	/** Status code for this packet */
	enum libusb_transfer_status status;
};

struct libusb_transfer;

/** \ingroup asyncio
 * Asynchronous transfer callback function type. When submitting asynchronous
 * transfers, you pass a pointer to a callback function of this type via the
 * \ref libusb_transfer::callback "callback" member of the libusb_transfer
 * structure. libusb will call this function later, when the transfer has
 * completed or failed. See \ref asyncio for more information.
 * \param transfer The libusb_transfer struct the callback function is being
 * notified about.
 */
typedef void (LIBUSB_CALL *libusb_transfer_cb_fn)(struct libusb_transfer *transfer);

/** \ingroup asyncio
 * The generic USB transfer structure. The user populates this structure and
 * then submits it in order to request a transfer. After the transfer has
 * completed, the library populates the transfer with the results and passes
 * it back to the user.
 */
struct libusb_transfer {
	/** Handle of the device that this transfer will be submitted to */
	libusb_device_handle *dev_handle;

	/** A bitwise OR combination of \ref libusb_transfer_flags. */
	uint8_t flags;

	/** Address of the endpoint where this transfer will be sent. */
	unsigned char endpoint;

	/** Type of the endpoint from \ref libusb_transfer_type */
	unsigned char type;

	/** Timeout for this transfer in millseconds. A value of 0 indicates no
	 * timeout. */
	unsigned int timeout;

	/** The status of the transfer. Read-only, and only for use within
	 * transfer callback function.
	 *
	 * If this is an isochronous transfer, this field may read COMPLETED even
	 * if there were errors in the frames. Use the
	 * \ref libusb_iso_packet_descriptor::status "status" field in each packet
	 * to determine if errors occurred. */
	enum libusb_transfer_status status;

	/** Length of the data buffer */
	int length;

	/** Actual length of data that was transferred. Read-only, and only for
	 * use within transfer callback function. Not valid for isochronous
	 * endpoint transfers. */
	int actual_length;

	/** Callback function. This will be invoked when the transfer completes,
	 * fails, or is cancelled. */
	libusb_transfer_cb_fn callback;

	/** User context data to pass to the callback function. */
	void *user_data;

	/** Data buffer */
	unsigned char *buffer;

	/** Number of isochronous packets. Only used for I/O with isochronous
	 * endpoints. */
	int num_iso_packets;

	/** Isochronous packet descriptors, for isochronous transfers only. */
	struct libusb_iso_packet_descriptor iso_packet_desc
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code */
#else
	[0] /* non-standard, but usually working code */
#endif
	;
};

/** \ingroup misc
 * Capabilities supported by this instance of libusb. Test if the loaded
 * library supports a given capability by calling
 * \ref libusb_has_capability().
 */
enum libusb_capability {
	/** The libusb_has_capability() API is available. */
	LIBUSB_CAP_HAS_CAPABILITY = 0,
	/** The libusb hotplug API is available. */
	LIBUSB_CAP_HAS_HOTPLUG    = 1,
};

int LIBUSB_CALL libusb_init(libusb_context **ctx);
void LIBUSB_CALL libusb_exit(libusb_context *ctx);
void LIBUSB_CALL libusb_set_debug(libusb_context *ctx, int level);
const struct libusb_version * LIBUSB_CALL libusb_get_version(void);
int LIBUSB_CALL libusb_has_capability(uint32_t capability);
const char * LIBUSB_CALL libusb_error_name(int errcode);
const char * LIBUSB_CALL libusb_strerror(enum libusb_error errcode);

ssize_t LIBUSB_CALL libusb_get_device_list(libusb_context *ctx,
	libusb_device ***list);
void LIBUSB_CALL libusb_free_device_list(libusb_device **list,
	int unref_devices);
libusb_device * LIBUSB_CALL libusb_ref_device(libusb_device *dev);
void LIBUSB_CALL libusb_unref_device(libusb_device *dev);

int LIBUSB_CALL libusb_get_configuration(libusb_device_handle *dev,
	int *config);
int LIBUSB_CALL libusb_get_device_descriptor(libusb_device *dev,
	struct libusb_device_descriptor *desc);
int LIBUSB_CALL libusb_get_active_config_descriptor(libusb_device *dev,
	struct libusb_config_descriptor **config);
int LIBUSB_CALL libusb_get_config_descriptor(libusb_device *dev,
	uint8_t config_index, struct libusb_config_descriptor **config);
int LIBUSB_CALL libusb_get_config_descriptor_by_value(libusb_device *dev,
	uint8_t bConfigurationValue, struct libusb_config_descriptor **config);
void LIBUSB_CALL libusb_free_config_descriptor(
	struct libusb_config_descriptor *config);
uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device *dev);
uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device *dev);
int LIBUSB_CALL libusb_get_device_speed(libusb_device *dev);
int LIBUSB_CALL libusb_get_max_packet_size(libusb_device *dev,
	unsigned char endpoint);
int LIBUSB_CALL libusb_get_max_iso_packet_size(libusb_device *dev,
	unsigned char endpoint);

int LIBUSB_CALL libusb_open(libusb_device *dev, libusb_device_handle **handle);
void LIBUSB_CALL libusb_close(libusb_device_handle *dev_handle);
libusb_device * LIBUSB_CALL libusb_get_device(libusb_device_handle *dev_handle);

int LIBUSB_CALL libusb_set_configuration(libusb_device_handle *dev,
	int configuration);
int LIBUSB_CALL libusb_claim_interface(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_release_interface(libusb_device_handle *dev,
	int interface_number);

libusb_device_handle * LIBUSB_CALL libusb_open_device_with_vid_pid(
	libusb_context *ctx, uint16_t vendor_id, uint16_t product_id);

int LIBUSB_CALL libusb_set_interface_alt_setting(libusb_device_handle *dev,
	int interface_number, int alternate_setting);
int LIBUSB_CALL libusb_clear_halt(libusb_device_handle *dev,
	unsigned char endpoint);
int LIBUSB_CALL libusb_reset_device(libusb_device_handle *dev);

int LIBUSB_CALL libusb_kernel_driver_active(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_detach_kernel_driver(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_attach_kernel_driver(libusb_device_handle *dev,
	int interface_number);

/* async I/O */

/** \ingroup asyncio
 * Get the data section of a control transfer. This convenience function is here
 * to remind you that the data does not start until 8 bytes into the actual
 * buffer, as the setup packet comes first.
 *
 * Calling this function only makes sense from a transfer callback function,
 * or situations where you have already allocated a suitably sized buffer at
 * transfer->buffer.
 *
 * \param transfer a transfer
 * \returns pointer to the first byte of the data section
 */
static inline unsigned char *libusb_control_transfer_get_data(
	struct libusb_transfer *transfer)
{
	return transfer->buffer + LIBUSB_CONTROL_SETUP_SIZE;
}

/** \ingroup asyncio
 * Get the control setup packet of a control transfer. This convenience
 * function is here to remind you that the control setup occupies the first
 * 8 bytes of the transfer data buffer.
 *
 * Calling this function only makes sense from a transfer callback function,
 * or situations where you have already allocated a suitably sized buffer at
 * transfer->buffer.
 *
 * \param transfer a transfer
 * \returns a casted pointer to the start of the transfer data buffer
 */
static inline struct libusb_control_setup *libusb_control_transfer_get_setup(
	struct libusb_transfer *transfer)
{
	return (struct libusb_control_setup *) transfer->buffer;
}

/** \ingroup asyncio
 * Helper function to populate the setup packet (first 8 bytes of the data
 * buffer) for a control transfer. The wIndex, wValue and wLength values should
 * be given in host-endian byte order.
 *
 * \param buffer buffer to output the setup packet into
 * \param bmRequestType see the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field of
 * \ref libusb_control_setup
 * \param bRequest see the
 * \ref libusb_control_setup::bRequest "bRequest" field of
 * \ref libusb_control_setup
 * \param wValue see the
 * \ref libusb_control_setup::wValue "wValue" field of
 * \ref libusb_control_setup
 * \param wIndex see the
 * \ref libusb_control_setup: