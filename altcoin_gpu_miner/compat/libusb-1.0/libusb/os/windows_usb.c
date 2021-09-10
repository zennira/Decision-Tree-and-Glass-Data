
/*
 * windows backend for libusb 1.0
 * Copyright (c) 2009-2010 Pete Batard <pbatard@gmail.com>
 * With contributions from Michael Plante, Orin Eman et al.
 * Parts of this code adapted from libusb-win32-v1 by Stephan Meyer
 * Hash table functions adapted from glibc, by Ulrich Drepper et al.
 * Major code testing contribution by Xiaofan Chen
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

#include <config.h>
#include <windows.h>
#include <setupapi.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <process.h>
#include <stdio.h>
#include <objbase.h>
#include <winioctl.h>

#include <libusbi.h>
#include "poll_windows.h"
#include "windows_usb.h"

// The following prevents "banned API" errors when using the MS's WDK OACR/Prefast
#if defined(_PREFAST_)
#pragma warning(disable:28719)
#endif

// The 2 macros below are used in conjunction with safe loops.
#define LOOP_CHECK(fcall) { r=fcall; if (r != LIBUSB_SUCCESS) continue; }
#define LOOP_BREAK(err) { r=err; continue; }

extern void usbi_fd_notification(struct libusb_context *ctx);

// Helper prototypes
static int windows_get_active_config_descriptor(struct libusb_device *dev, unsigned char *buffer, size_t len, int *host_endian);
static int windows_clock_gettime(int clk_id, struct timespec *tp);
unsigned __stdcall windows_clock_gettime_threaded(void* param);
// WinUSB API prototypes
static int winusb_init(struct libusb_context *ctx);
static int winusb_exit(void);
static int winusb_open(struct libusb_device_handle *dev_handle);
static void winusb_close(struct libusb_device_handle *dev_handle);
static int winusb_configure_endpoints(struct libusb_device_handle *dev_handle, int iface);
static int winusb_claim_interface(struct libusb_device_handle *dev_handle, int iface);
static int winusb_release_interface(struct libusb_device_handle *dev_handle, int iface);
static int winusb_submit_control_transfer(struct usbi_transfer *itransfer);
static int winusb_set_interface_altsetting(struct libusb_device_handle *dev_handle, int iface, int altsetting);
static int winusb_submit_bulk_transfer(struct usbi_transfer *itransfer);
static int winusb_clear_halt(struct libusb_device_handle *dev_handle, unsigned char endpoint);
static int winusb_abort_transfers(struct usbi_transfer *itransfer);
static int winusb_abort_control(struct usbi_transfer *itransfer);
static int winusb_reset_device(struct libusb_device_handle *dev_handle);
static int winusb_copy_transfer_data(struct usbi_transfer *itransfer, uint32_t io_size);
// Composite API prototypes
static int composite_init(struct libusb_context *ctx);
static int composite_exit(void);
static int composite_open(struct libusb_device_handle *dev_handle);
static void composite_close(struct libusb_device_handle *dev_handle);
static int composite_claim_interface(struct libusb_device_handle *dev_handle, int iface);
static int composite_set_interface_altsetting(struct libusb_device_handle *dev_handle, int iface, int altsetting);
static int composite_release_interface(struct libusb_device_handle *dev_handle, int iface);
static int composite_submit_control_transfer(struct usbi_transfer *itransfer);
static int composite_submit_bulk_transfer(struct usbi_transfer *itransfer);
static int composite_submit_iso_transfer(struct usbi_transfer *itransfer);
static int composite_clear_halt(struct libusb_device_handle *dev_handle, unsigned char endpoint);
static int composite_abort_transfers(struct usbi_transfer *itransfer);
static int composite_abort_control(struct usbi_transfer *itransfer);
static int composite_reset_device(struct libusb_device_handle *dev_handle);
static int composite_copy_transfer_data(struct usbi_transfer *itransfer, uint32_t io_size);


// Global variables
uint64_t hires_frequency, hires_ticks_to_ps;
const uint64_t epoch_time = UINT64_C(116444736000000000);	// 1970.01.01 00:00:000 in MS Filetime
enum windows_version windows_version = WINDOWS_UNSUPPORTED;
// Concurrency
static int concurrent_usage = -1;
usbi_mutex_t autoclaim_lock;
// Timer thread
// NB: index 0 is for monotonic and 1 is for the thread exit event
HANDLE timer_thread = NULL;
HANDLE timer_mutex = NULL;
struct timespec timer_tp;
volatile LONG request_count[2] = {0, 1};	// last one must be > 0
HANDLE timer_request[2] = { NULL, NULL };
HANDLE timer_response = NULL;
// API globals
bool api_winusb_available = false;
#define CHECK_WINUSB_AVAILABLE do { if (!api_winusb_available) return LIBUSB_ERROR_ACCESS; } while (0)

static inline BOOLEAN guid_eq(const GUID *guid1, const GUID *guid2) {
	if ((guid1 != NULL) && (guid2 != NULL)) {
		return (memcmp(guid1, guid2, sizeof(GUID)) == 0);
	}
	return false;
}

#if defined(ENABLE_DEBUG_LOGGING) || (defined(_MSC_VER) && _MSC_VER < 1400)
static char* guid_to_string(const GUID* guid)
{
	static char guid_string[MAX_GUID_STRING_LENGTH];

	if (guid == NULL) return NULL;
	sprintf(guid_string, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		(unsigned int)guid->Data1, guid->Data2, guid->Data3,
		guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
		guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
	return guid_string;
}
#endif

/*
 * Converts a windows error to human readable string
 * uses retval as errorcode, or, if 0, use GetLastError()
 */
static char *windows_error_str(uint32_t retval)
{
static char err_string[ERR_BUFFER_SIZE];

	DWORD size;
	size_t i;
	uint32_t error_code, format_error;

	error_code = retval?retval:GetLastError();

	safe_sprintf(err_string, ERR_BUFFER_SIZE, "[%d] ", error_code);

	size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &err_string[safe_strlen(err_string)],
		ERR_BUFFER_SIZE - (DWORD)safe_strlen(err_string), NULL);
	if (size == 0) {
		format_error = GetLastError();
		if (format_error)
			safe_sprintf(err_string, ERR_BUFFER_SIZE,
				"Windows error code %u (FormatMessage error code %u)", error_code, format_error);
		else
			safe_sprintf(err_string, ERR_BUFFER_SIZE, "Unknown error code %u", error_code);
	} else {
		// Remove CR/LF terminators
		for (i=safe_strlen(err_string)-1; ((err_string[i]==0x0A) || (err_string[i]==0x0D)); i--) {
			err_string[i] = 0;
		}
	}
	return err_string;
}

/*
 * Sanitize Microsoft's paths: convert to uppercase, add prefix and fix backslashes.
 * Return an allocated sanitized string or NULL on error.
 */
static char* sanitize_path(const char* path)
{
	const char root_prefix[] = "\\\\.\\";
	size_t j, size, root_size;
	char* ret_path = NULL;
	size_t add_root = 0;

	if (path == NULL)
		return NULL;

	size = safe_strlen(path)+1;
	root_size = sizeof(root_prefix)-1;

	// Microsoft indiscriminatly uses '\\?\', '\\.\', '##?#" or "##.#" for root prefixes.
	if (!((size > 3) && (((path[0] == '\\') && (path[1] == '\\') && (path[3] == '\\')) ||
		((path[0] == '#') && (path[1] == '#') && (path[3] == '#'))))) {
		add_root = root_size;
		size += add_root;
	}

	if ((ret_path = (char*)calloc(size, 1)) == NULL)
		return NULL;

	safe_strcpy(&ret_path[add_root], size-add_root, path);

	// Ensure consistancy with root prefix
	for (j=0; j<root_size; j++)
		ret_path[j] = root_prefix[j];

	// Same goes for '\' and '#' after the root prefix. Ensure '#' is used
	for(j=root_size; j<size; j++) {
		ret_path[j] = (char)toupper((int)ret_path[j]);	// Fix case too
		if (ret_path[j] == '\\')
			ret_path[j] = '#';
	}

	return ret_path;
}

/*
 * Cfgmgr32, OLE32 and SetupAPI DLL functions
 */
static int init_dlls(void)
{
	DLL_LOAD(Cfgmgr32.dll, CM_Get_Parent, TRUE);
	DLL_LOAD(Cfgmgr32.dll, CM_Get_Child, TRUE);
	DLL_LOAD(Cfgmgr32.dll, CM_Get_Sibling, TRUE);
	DLL_LOAD(Cfgmgr32.dll, CM_Get_Device_IDA, TRUE);
	// Prefixed to avoid conflict with header files
	DLL_LOAD_PREFIXED(OLE32.dll, p, CLSIDFromString, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiGetClassDevsA, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiEnumDeviceInfo, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiEnumDeviceInterfaces, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiGetDeviceInterfaceDetailA, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiDestroyDeviceInfoList, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiOpenDevRegKey, TRUE);
	DLL_LOAD_PREFIXED(SetupAPI.dll, p, SetupDiGetDeviceRegistryPropertyA, TRUE);
	DLL_LOAD_PREFIXED(AdvAPI32.dll, p, RegQueryValueExW, TRUE);
	DLL_LOAD_PREFIXED(AdvAPI32.dll, p, RegCloseKey, TRUE);
	return LIBUSB_SUCCESS;
}

/*
 * enumerate interfaces for the whole USB class
 *
 * Parameters:
 * dev_info: a pointer to a dev_info list
 * dev_info_data: a pointer to an SP_DEVINFO_DATA to be filled (or NULL if not needed)
 * usb_class: the generic USB class for which to retrieve interface details
 * index: zero based index of the interface in the device info list
 *
 * Note: it is the responsibility of the caller to free the DEVICE_INTERFACE_DETAIL_DATA
 * structure returned and call this function repeatedly using the same guid (with an
 * incremented index starting at zero) until all interfaces have been returned.
 */
static bool get_devinfo_data(struct libusb_context *ctx,
	HDEVINFO *dev_info, SP_DEVINFO_DATA *dev_info_data, char* usb_class, unsigned _index)
{
	if (_index <= 0) {
		*dev_info = pSetupDiGetClassDevsA(NULL, usb_class, NULL, DIGCF_PRESENT|DIGCF_ALLCLASSES);
		if (*dev_info == INVALID_HANDLE_VALUE) {
			return false;
		}
	}

	dev_info_data->cbSize = sizeof(SP_DEVINFO_DATA);
	if (!pSetupDiEnumDeviceInfo(*dev_info, _index, dev_info_data)) {
		if (GetLastError() != ERROR_NO_MORE_ITEMS) {
			usbi_err(ctx, "Could not obtain device info data for index %u: %s",
				_index, windows_error_str(0));
		}
		pSetupDiDestroyDeviceInfoList(*dev_info);
		*dev_info = INVALID_HANDLE_VALUE;
		return false;
	}
	return true;
}

/*
 * enumerate interfaces for a specific GUID
 *
 * Parameters:
 * dev_info: a pointer to a dev_info list
 * dev_info_data: a pointer to an SP_DEVINFO_DATA to be filled (or NULL if not needed)
 * guid: the GUID for which to retrieve interface details
 * index: zero based index of the interface in the device info list
 *
 * Note: it is the responsibility of the caller to free the DEVICE_INTERFACE_DETAIL_DATA
 * structure returned and call this function repeatedly using the same guid (with an
 * incremented index starting at zero) until all interfaces have been returned.
 */
static SP_DEVICE_INTERFACE_DETAIL_DATA_A *get_interface_details(struct libusb_context *ctx,
	HDEVINFO *dev_info, SP_DEVINFO_DATA *dev_info_data, const GUID* guid, unsigned _index)
{
	SP_DEVICE_INTERFACE_DATA dev_interface_data;
	SP_DEVICE_INTERFACE_DETAIL_DATA_A *dev_interface_details = NULL;
	DWORD size;

	if (_index <= 0) {
		*dev_info = pSetupDiGetClassDevsA(guid, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
	}

	if (dev_info_data != NULL) {
		dev_info_data->cbSize = sizeof(SP_DEVINFO_DATA);
		if (!pSetupDiEnumDeviceInfo(*dev_info, _index, dev_info_data)) {
			if (GetLastError() != ERROR_NO_MORE_ITEMS) {
				usbi_err(ctx, "Could not obtain device info data for index %u: %s",
					_index, windows_error_str(0));
			}
			pSetupDiDestroyDeviceInfoList(*dev_info);
			*dev_info = INVALID_HANDLE_VALUE;
			return NULL;
		}
	}

	dev_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (!pSetupDiEnumDeviceInterfaces(*dev_info, NULL, guid, _index, &dev_interface_data)) {
		if (GetLastError() != ERROR_NO_MORE_ITEMS) {
			usbi_err(ctx, "Could not obtain interface data for index %u: %s",
				_index, windows_error_str(0));
		}
		pSetupDiDestroyDeviceInfoList(*dev_info);
		*dev_info = INVALID_HANDLE_VALUE;
		return NULL;
	}

	// Read interface data (dummy + actual) to access the device path
	if (!pSetupDiGetDeviceInterfaceDetailA(*dev_info, &dev_interface_data, NULL, 0, &size, NULL)) {
		// The dummy call should fail with ERROR_INSUFFICIENT_BUFFER
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			usbi_err(ctx, "could not access interface data (dummy) for index %u: %s",
				_index, windows_error_str(0));
			goto err_exit;
		}
	} else {
		usbi_err(ctx, "program assertion failed - http://msdn.microsoft.com/en-us/library/ms792901.aspx is wrong.");
		goto err_exit;
	}

	if ((dev_interface_details = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*) calloc(size, 1)) == NULL) {
		usbi_err(ctx, "could not allocate interface data for index %u.", _index);
		goto err_exit;
	}

	dev_interface_details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
	if (!pSetupDiGetDeviceInterfaceDetailA(*dev_info, &dev_interface_data,
		dev_interface_details, size, &size, NULL)) {
		usbi_err(ctx, "could not access interface data (actual) for index %u: %s",
			_index, windows_error_str(0));
	}

	return dev_interface_details;

err_exit:
	pSetupDiDestroyDeviceInfoList(*dev_info);
	*dev_info = INVALID_HANDLE_VALUE;
	return NULL;
}

/* Hash table functions - modified From glibc 2.3.2:
   [Aho,Sethi,Ullman] Compilers: Principles, Techniques and Tools, 1986
   [Knuth]            The Art of Computer Programming, part 3 (6.4)  */
typedef struct htab_entry {
	unsigned long used;
	char* str;
} htab_entry;
htab_entry* htab_table = NULL;
usbi_mutex_t htab_write_mutex = NULL;
unsigned long htab_size, htab_filled;

/* For the used double hash method the table size has to be a prime. To
   correct the user given table size we need a prime test.  This trivial
   algorithm is adequate because the code is called only during init and
   the number is likely to be small  */
static int isprime(unsigned long number)
{
	// no even number will be passed
	unsigned int divider = 3;

	while((divider * divider < number) && (number % divider != 0))
		divider += 2;

	return (number % divider != 0);
}

/* Before using the hash table we must allocate memory for it.
   We allocate one element more as the found prime number says.
   This is done for more effective indexing as explained in the
   comment for the hash function.  */
static int htab_create(struct libusb_context *ctx, unsigned long nel)
{
	if (htab_table != NULL) {
		usbi_err(ctx, "hash table already allocated");
	}

	// Create a mutex
	usbi_mutex_init(&htab_write_mutex, NULL);

	// Change nel to the first prime number not smaller as nel.
	nel |= 1;
	while(!isprime(nel))
		nel += 2;

	htab_size = nel;
	usbi_dbg("using %d entries hash table", nel);
	htab_filled = 0;

	// allocate memory and zero out.
	htab_table = (htab_entry*)calloc(htab_size + 1, sizeof(htab_entry));
	if (htab_table == NULL) {
		usbi_err(ctx, "could not allocate space for hash table");
		return 0;
	}

	return 1;
}

/* After using the hash table it has to be destroyed.  */
static void htab_destroy(void)
{
	size_t i;
	if (htab_table == NULL) {
		return;
	}

	for (i=0; i<htab_size; i++) {
		if (htab_table[i].used) {
			safe_free(htab_table[i].str);
		}
	}
	usbi_mutex_destroy(&htab_write_mutex);
	safe_free(htab_table);
}

/* This is the search function. It uses double hashing with open addressing.
   We use an trick to speed up the lookup. The table is created with one
   more element available. This enables us to use the index zero special.
   This index will never be used because we store the first hash index in
   the field used where zero means not used. Every other value means used.
   The used field can be used as a first fast comparison for equality of
   the stored and the parameter value. This helps to prevent unnecessary
   expensive calls of strcmp.  */
static unsigned long htab_hash(char* str)
{
	unsigned long hval, hval2;
	unsigned long idx;
	unsigned long r = 5381;
	int c;
	char* sz = str;

	// Compute main hash value (algorithm suggested by Nokia)
	while ((c = *sz++))
		r = ((r << 5) + r) + c;
	if (r == 0)
		++r;

	// compute table hash: simply take the modulus
	hval = r % htab_size;
	if (hval == 0)
		++hval;

	// Try the first index
	idx = hval;

	if (htab_table[idx].used) {
		if ( (htab_table[idx].used == hval)
		  && (safe_strcmp(str, htab_table[idx].str) == 0) ) {
			// existing hash
			return idx;
		}
		usbi_dbg("hash collision ('%s' vs '%s')", str, htab_table[idx].str);

		// Second hash function, as suggested in [Knuth]
		hval2 = 1 + hval % (htab_size - 2);

		do {
			// Because size is prime this guarantees to step through all available indexes
			if (idx <= hval2) {
				idx = htab_size + idx - hval2;
			} else {
				idx -= hval2;
			}

			// If we visited all entries leave the loop unsuccessfully
			if (idx == hval) {
				break;
			}

			// If entry is found use it.
			if ( (htab_table[idx].used == hval)
			  && (safe_strcmp(str, htab_table[idx].str) == 0) ) {
				return idx;
			}
		}
		while (htab_table[idx].used);
	}

	// Not found => New entry

	// If the table is full return an error
	if (htab_filled >= htab_size) {
		usbi_err(NULL, "hash table is full (%d entries)", htab_size);
		return 0;
	}

	// Concurrent threads might be storing the same entry at the same time
	// (eg. "simultaneous" enums from different threads) => use a mutex
	usbi_mutex_lock(&htab_write_mutex);
	// Just free any previously allocated string (which should be the same as
	// new one). The possibility of concurrent threads storing a collision
	// string (same hash, different string) at the same time is extremely low
	safe_free(htab_table[idx].str);
	htab_table[idx].used = hval;
	htab_table[idx].str = (char*) calloc(1, safe_strlen(str)+1);
	if (htab_table[idx].str == NULL) {
		usbi_err(NULL, "could not duplicate string for hash table");
		usbi_mutex_unlock(&htab_write_mutex);
		return 0;
	}
	memcpy(htab_table[idx].str, str, safe_strlen(str)+1);
	++htab_filled;
	usbi_mutex_unlock(&htab_write_mutex);

	return idx;
}

/*
 * Returns the session ID of a device's nth level ancestor
 * If there's no device at the nth level, return 0
 */
static unsigned long get_ancestor_session_id(DWORD devinst, unsigned level)
{
	DWORD parent_devinst;
	unsigned long session_id = 0;
	char* sanitized_path = NULL;
	char path[MAX_PATH_LENGTH];
	unsigned i;

	if (level < 1) return 0;
	for (i = 0; i<level; i++) {
		if (CM_Get_Parent(&parent_devinst, devinst, 0) != CR_SUCCESS) {
			return 0;
		}
		devinst = parent_devinst;
	}
	if (CM_Get_Device_IDA(devinst, path, MAX_PATH_LENGTH, 0) != CR_SUCCESS) {
		return 0;
	}
	// TODO (post hotplug): try without sanitizing
	sanitized_path = sanitize_path(path);
	if (sanitized_path == NULL) {
		return 0;
	}
	session_id = htab_hash(sanitized_path);
	safe_free(sanitized_path);
	return session_id;
}

/*
 * Populate the endpoints addresses of the device_priv interface helper structs
 */
static int windows_assign_endpoints(struct libusb_device_handle *dev_handle, int iface, int altsetting)
{
	int i, r;
	struct windows_device_priv *priv = _device_priv(dev_handle->dev);
	struct libusb_config_descriptor *conf_desc;
	const struct libusb_interface_descriptor *if_desc;
	struct libusb_context *ctx = DEVICE_CTX(dev_handle->dev);

	r = libusb_get_config_descriptor(dev_handle->dev, 0, &conf_desc);
	if (r != LIBUSB_SUCCESS) {
		usbi_warn(ctx, "could not read config descriptor: error %d", r);
		return r;
	}

	if_desc = &conf_desc->interface[iface].altsetting[altsetting];
	safe_free(priv->usb_interface[iface].endpoint);

	if (if_desc->bNumEndpoints == 0) {
		usbi_dbg("no endpoints found for interface %d", iface);
		return LIBUSB_SUCCESS;
	}

	priv->usb_interface[iface].endpoint = (uint8_t*) calloc(1, if_desc->bNumEndpoints);
	if (priv->usb_interface[iface].endpoint == NULL) {
		return LIBUSB_ERROR_NO_MEM;
	}

	priv->usb_interface[iface].nb_endpoints = if_desc->bNumEndpoints;
	for (i=0; i<if_desc->bNumEndpoints; i++) {
		priv->usb_interface[iface].endpoint[i] = if_desc->endpoint[i].bEndpointAddress;
		usbi_dbg("(re)assigned endpoint %02X to interface %d", priv->usb_interface[iface].endpoint[i], iface);
	}
	libusb_free_config_descriptor(conf_desc);

	// Extra init is required for WinUSB endpoints
	if (priv->apib->id == USB_API_WINUSB) {
		return winusb_configure_endpoints(dev_handle, iface);
	}

	return LIBUSB_SUCCESS;
}

// Lookup for a match in the list of API driver names
static bool is_api_driver(char* driver, uint8_t api)
{
	uint8_t i;
	const char sep_str[2] = {LIST_SEPARATOR, 0};
	char *tok, *tmp_str;
	size_t len = safe_strlen(driver);

	if (len == 0) return false;
	tmp_str = (char*) calloc(1, len+1);
	if (tmp_str == NULL) return false;
	memcpy(tmp_str, driver, len+1);
	tok = strtok(tmp_str, sep_str);
	while (tok != NULL) {
		for (i=0; i<usb_api_backend[api].nb_driver_names; i++) {
			if (safe_strcmp(tok, usb_api_backend[api].driver_name_list[i]) == 0) {
				free(tmp_str);
				return true;
			}
		}
		tok = strtok(NULL, sep_str);
	}
	free (tmp_str);
	return false;
}

/*
 * auto-claiming and auto-release helper functions
 */
static int auto_claim(struct libusb_transfer *transfer, int *interface_number, int api_type)
{
	struct libusb_context *ctx = DEVICE_CTX(transfer->dev_handle->dev);
	struct windows_device_handle_priv *handle_priv = _device_handle_priv(
		transfer->dev_handle);
	struct windows_device_priv *priv = _device_priv(transfer->dev_handle->dev);
	int current_interface = *interface_number;
	int r = LIBUSB_SUCCESS;

	usbi_mutex_lock(&autoclaim_lock);
	if (current_interface < 0)	// No serviceable interface was found
	{
		for (current_interface=0; current_interface<USB_MAXINTERFACES; current_interface++) {
			// Must claim an interface of the same API type
			if ( (priv->usb_interface[current_interface].apib->id == api_type)
			  && (libusb_claim_interface(transfer->dev_handle, current_interface) == LIBUSB_SUCCESS) ) {
				usbi_dbg("auto-claimed interface %d for control request", current_interface);
				if (handle_priv->autoclaim_count[current_interface] != 0) {
					usbi_warn(ctx, "program assertion failed - autoclaim_count was nonzero");
				}
				handle_priv->autoclaim_count[current_interface]++;
				break;
			}
		}
		if (current_interface == USB_MAXINTERFACES) {
			usbi_err(ctx, "could not auto-claim any interface");
			r = LIBUSB_ERROR_NOT_FOUND;
		}
	} else {
		// If we have a valid interface that was autoclaimed, we must increment
		// its autoclaim count so that we can prevent an early release.
		if (handle_priv->autoclaim_count[current_interface] != 0) {
			handle_priv->autoclaim_count[current_interface]++;
		}
	}
	usbi_mutex_unlock(&autoclaim_lock);

	*interface_number = current_interface;
	return r;

}

static void auto_release(struct usbi_transfer *itransfer)
{
	struct windows_transfer_priv *transfer_priv = (struct windows_transfer_priv*)usbi_transfer_get_os_priv(itransfer);
	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	libusb_device_handle *dev_handle = transfer->dev_handle;
	struct windows_device_handle_priv* handle_priv = _device_handle_priv(dev_handle);
	int r;

	usbi_mutex_lock(&autoclaim_lock);
	if (handle_priv->autoclaim_count[transfer_priv->interface_number] > 0) {
		handle_priv->autoclaim_count[transfer_priv->interface_number]--;
		if (handle_priv->autoclaim_count[transfer_priv->interface_number] == 0) {
			r = libusb_release_interface(dev_handle, transfer_priv->interface_number);
			if (r == LIBUSB_SUCCESS) {
				usbi_dbg("auto-released interface %d", transfer_priv->interface_number);
			} else {
				usbi_dbg("failed to auto-release interface %d (%s)",
					transfer_priv->interface_number, libusb_error_name((enum libusb_error)r));
			}
		}
	}
	usbi_mutex_unlock(&autoclaim_lock);
}

/*
 * init: libusb backend init function
 *
 * This function enumerates the HCDs (Host Controller Drivers) and populates our private HCD list
 * In our implementation, we equate Windows' "HCD" to LibUSB's "bus". Note that bus is zero indexed.
 * HCDs are not expected to change after init (might not hold true for hot pluggable USB PCI card?)
 */
static int windows_init(struct libusb_context *ctx)
{
	int i, r = LIBUSB_ERROR_OTHER;
	OSVERSIONINFO os_version;
	HANDLE semaphore;
	char sem_name[11+1+8]; // strlen(libusb_init)+'\0'+(32-bit hex PID)

	sprintf(sem_name, "libusb_init%08X", (unsigned int)GetCurrentProcessId()&0xFFFFFFFF);
	semaphore = CreateSemaphoreA(NULL, 1, 1, sem_name);
	if (semaphore == NULL) {
		usbi_err(ctx, "could not create semaphore: %s", windows_error_str(0));
		return LIBUSB_ERROR_NO_MEM;
	}

	// A successful wait brings our semaphore count to 0 (unsignaled)
	// => any concurent wait stalls until the semaphore's release
	if (WaitForSingleObject(semaphore, INFINITE) != WAIT_OBJECT_0) {
		usbi_err(ctx, "failure to access semaphore: %s", windows_error_str(0));
		CloseHandle(semaphore);
		return LIBUSB_ERROR_NO_MEM;
	}

	// NB: concurrent usage supposes that init calls are equally balanced with
	// exit calls. If init is called more than exit, we will not exit properly
	if ( ++concurrent_usage == 0 ) {	// First init?
		// Detect OS version
		memset(&os_version, 0, sizeof(OSVERSIONINFO));
		os_version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		windows_version = WINDOWS_UNSUPPORTED;
		if ((GetVersionEx(&os_version) != 0) && (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT)) {
			if ((os_version.dwMajorVersion == 5) && (os_version.dwMinorVersion == 1)) {
				windows_version = WINDOWS_XP;
			} else if ((os_version.dwMajorVersion == 5) && (os_version.dwMinorVersion == 2)) {
				windows_version = WINDOWS_2003;	// also includes XP 64
			} else if (os_version.dwMajorVersion >= 6) {
				windows_version = WINDOWS_VISTA_AND_LATER;
			}
		}
		if (windows_version == WINDOWS_UNSUPPORTED) {
			usbi_err(ctx, "This version of Windows is NOT supported");
			r = LIBUSB_ERROR_NOT_SUPPORTED;
			goto init_exit;
		}

		// We need a lock for proper auto-release
		usbi_mutex_init(&autoclaim_lock, NULL);

		// Initialize pollable file descriptors
		init_polling();

		// Load DLL imports
		if (init_dlls() != LIBUSB_SUCCESS) {
			usbi_err(ctx, "could not resolve DLL functions");
			return LIBUSB_ERROR_NOT_FOUND;
		}

		// Initialize the low level APIs (we don't care about errors at this stage)
		for (i=0; i<USB_API_MAX; i++) {
			usb_api_backend[i].init(ctx);
		}

		// Because QueryPerformanceCounter might report different values when
		// running on different cores, we create a separate thread for the timer
		// calls, which we glue to the first core always to prevent timing discrepancies.
		r = LIBUSB_ERROR_NO_MEM;
		for (i = 0; i < 2; i++) {
			timer_request[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (timer_request[i] == NULL) {
				usbi_err(ctx, "could not create timer request event %d - aborting", i);
				goto init_exit;
			}
		}
		timer_response = CreateSemaphore(NULL, 0, MAX_TIMER_SEMAPHORES, NULL);
		if (timer_response == NULL) {
			usbi_err(ctx, "could not create timer response semaphore - aborting");
			goto init_exit;
		}
		timer_mutex = CreateMutex(NULL, FALSE, NULL);
		if (timer_mutex == NULL) {
			usbi_err(ctx, "could not create timer mutex - aborting");
			goto init_exit;
		}
		timer_thread = (HANDLE)_beginthreadex(NULL, 0, windows_clock_gettime_threaded, NULL, 0, NULL);
		if (timer_thread == NULL) {
			usbi_err(ctx, "Unable to create timer thread - aborting");
			goto init_exit;
		}
		SetThreadAffinityMask(timer_thread, 0);

		// Create a hash table to store session ids. Second parameter is better if prime
		htab_create(ctx, HTAB_SIZE);
	}
	// At this stage, either we went through full init successfully, or didn't need to
	r = LIBUSB_SUCCESS;

init_exit: // Holds semaphore here.
	if (!concurrent_usage && r != LIBUSB_SUCCESS) { // First init failed?
		if (timer_thread) {
			SetEvent(timer_request[1]); // actually the signal to quit the thread.
			if (WAIT_OBJECT_0 != WaitForSingleObject(timer_thread, INFINITE)) {
				usbi_warn(ctx, "could not wait for timer thread to quit");
				TerminateThread(timer_thread, 1); // shouldn't happen, but we're destroying
												  // all objects it might have held anyway.
			}
			CloseHandle(timer_thread);
			timer_thread = NULL;
		}
		for (i = 0; i < 2; i++) {
			if (timer_request[i]) {
				CloseHandle(timer_request[i]);
				timer_request[i] = NULL;
			}
		}
		if (timer_response) {
			CloseHandle(timer_response);
			timer_response = NULL;
		}
		if (timer_mutex) {
			CloseHandle(timer_mutex);
			timer_mutex = NULL;
		}
		htab_destroy();
	}

	if (r != LIBUSB_SUCCESS)
		--concurrent_usage; // Not expected to call libusb_exit if we failed.

	ReleaseSemaphore(semaphore, 1, NULL);	// increase count back to 1
	CloseHandle(semaphore);
	return r;
}

/*
 * HCD (root) hubs need to have their device descriptor manually populated
 *
 * Note that, like Microsoft does in the device manager, we populate the
 * Vendor and Device ID for HCD hubs with the ones from the PCI HCD device.
 */
static int force_hcd_device_descriptor(struct libusb_device *dev)
{
	struct windows_device_priv *parent_priv, *priv = _device_priv(dev);
	struct libusb_context *ctx = DEVICE_CTX(dev);
	int vid, pid;

	dev->num_configurations = 1;
	priv->dev_descriptor.bLength = sizeof(USB_DEVICE_DESCRIPTOR);
	priv->dev_descriptor.bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
	priv->dev_descriptor.bNumConfigurations = 1;
	priv->active_config = 1;

	if (priv->parent_dev == NULL) {
		usbi_err(ctx, "program assertion failed - HCD hub has no parent");
		return LIBUSB_ERROR_NO_DEVICE;
	}
	parent_priv = _device_priv(priv->parent_dev);
	if (sscanf(parent_priv->path, "\\\\.\\PCI#VEN_%04x&DEV_%04x%*s", &vid, &pid) == 2) {
		priv->dev_descriptor.idVendor = (uint16_t)vid;
		priv->dev_descriptor.idProduct = (uint16_t)pid;
	} else {
		usbi_warn(ctx, "could not infer VID/PID of HCD hub from '%s'", parent_priv->path);
		priv->dev_descriptor.idVendor = 0x1d6b;		// Linux Foundation root hub
		priv->dev_descriptor.idProduct = 1;
	}
	return LIBUSB_SUCCESS;
}

/*
 * fetch and cache all the config descriptors through I/O
 */
static int cache_config_descriptors(struct libusb_device *dev, HANDLE hub_handle, char* device_id)
{
	DWORD size, ret_size;
	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct windows_device_priv *priv = _device_priv(dev);
	int r;
	uint8_t i;

	USB_CONFIGURATION_DESCRIPTOR_SHORT cd_buf_short;    // dummy request
	PUSB_DESCRIPTOR_REQUEST cd_buf_actual = NULL;       // actual request
	PUSB_CONFIGURATION_DESCRIPTOR cd_data = NULL;

	if (dev->num_configurations == 0)
		return LIBUSB_ERROR_INVALID_PARAM;

	priv->config_descriptor = (unsigned char**) calloc(dev->num_configurations, sizeof(PUSB_CONFIGURATION_DESCRIPTOR));
	if (priv->config_descriptor == NULL)
		return LIBUSB_ERROR_NO_MEM;
	for (i=0; i<dev->num_configurations; i++)
		priv->config_descriptor[i] = NULL;

	for (i=0, r=LIBUSB_SUCCESS; ; i++)
	{
		// safe loop: release all dynamic resources
		safe_free(cd_buf_actual);

		// safe loop: end of loop condition
		if ((i >= dev->num_configurations) || (r != LIBUSB_SUCCESS))
			break;

		size = sizeof(USB_CONFIGURATION_DESCRIPTOR_SHORT);
		memset(&cd_buf_short, 0, size);

		cd_buf_short.req.ConnectionIndex = (ULONG)priv->port;
		cd_buf_short.req.SetupPacket.bmRequest = LIBUSB_ENDPOINT_IN;
		cd_buf_short.req.SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
		cd_buf_short.req.SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | i;
		cd_buf_short.req.SetupPacket.wIndex = i;
		cd_buf_short.req.SetupPacket.wLength = (USHORT)(size - sizeof(USB_DESCRIPTOR_REQUEST));

		// Dummy call to get the required data size
		if (!DeviceIoControl(hub_handle, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, &cd_buf_short, size,
			&cd_buf_short, size, &ret_size, NULL)) {
			usbi_err(ctx, "could not access configuration descriptor (dummy) for '%s': %s", device_id, windows_error_str(0));
			LOOP_BREAK(LIBUSB_ERROR_IO);
		}

		if ((ret_size != size) || (cd_buf_short.data.wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))) {
			usbi_err(ctx, "unexpected configuration descriptor size (dummy) for '%s'.", device_id);
			LOOP_BREAK(LIBUSB_ERROR_IO);
		}

		size = sizeof(USB_DESCRIPTOR_REQUEST) + cd_buf_short.data.wTotalLength;
		if ((cd_buf_actual = (PUSB_DESCRIPTOR_REQUEST) calloc(1, size)) == NULL) {
			usbi_err(ctx, "could not allocate configuration descriptor buffer for '%s'.", device_id);
			LOOP_BREAK(LIBUSB_ERROR_NO_MEM);
		}
		memset(cd_buf_actual, 0, size);

		// Actual call
		cd_buf_actual->ConnectionIndex = (ULONG)priv->port;
		cd_buf_actual->SetupPacket.bmRequest = LIBUSB_ENDPOINT_IN;
		cd_buf_actual->SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
		cd_buf_actual->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | i;
		cd_buf_actual->SetupPacket.wIndex = i;
		cd_buf_actual->SetupPacket.wLength = (USHORT)(size - sizeof(USB_DESCRIPTOR_REQUEST));

		if (!DeviceIoControl(hub_handle, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, cd_buf_actual, size,
			cd_buf_actual, size, &ret_size, NULL)) {
			usbi_err(ctx, "could not access configuration descriptor (actual) for '%s': %s", device_id, windows_error_str(0));
			LOOP_BREAK(LIBUSB_ERROR_IO);
		}

		cd_data = (PUSB_CONFIGURATION_DESCRIPTOR)((UCHAR*)cd_buf_actual+sizeof(USB_DESCRIPTOR_REQUEST));

		if ((size != ret_size) || (cd_data->wTotalLength != cd_buf_short.data.wTotalLength)) {
			usbi_err(ctx, "unexpected configuration descriptor size (actual) for '%s'.", device_id);
			LOOP_BREAK(LIBUSB_ERROR_IO);
		}

		if (cd_data->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE) {
			usbi_err(ctx, "not a configuration descriptor for '%s'", device_id);
			LOOP_BREAK(LIBUSB_ERROR_IO);
		}

		usbi_dbg("cached config descriptor %d (bConfigurationValue=%d, %d bytes)",
			i, cd_data->bConfigurationValue, cd_data->wTotalLength);

		// Cache the descriptor
		priv->config_descriptor[i] = (unsigned char*) calloc(1, cd_data->wTotalLength);
		if (priv->config_descriptor[i] == NULL)
			return LIBUSB_ERROR_NO_MEM;
		memcpy(priv->config_descriptor[i], cd_data, cd_data->wTotalLength);
	}
	return LIBUSB_SUCCESS;
}

/*
 * Populate a libusb device structure
 */
static int init_device(struct libusb_device* dev, struct libusb_device* parent_dev,
					   uint8_t port_number, char* device_id, DWORD devinst)
{
	HANDLE handle;
	DWORD size;
	USB_NODE_CONNECTION_INFORMATION_EX conn_info;
	struct windows_device_priv *priv, *parent_priv;
	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct libusb_device* tmp_dev;
	unsigned i;

	if ((dev == NULL) || (parent_dev == NULL)) {
		return LIBUSB_ERROR_NOT_FOUND;
	}
	priv = _device_priv(dev);
	parent_priv = _device_priv(parent_dev);
	if (parent_priv->apib->id != USB_API_HUB) {
		usbi_warn(ctx, "parent for device '%s' is not a hub", device_id);
		return LIBUSB_ERROR_NOT_FOUND;
	}

	// It is possible for the parent hub not to have been initialized yet
	// If that's the case, lookup the ancestors to set the bus number
	if (parent_dev->bus_number == 0) {
		for (i=2; ; i++) {
			tmp_dev = usbi_get_device_by_session_id(ctx, get_ancestor_session_id(devinst, i));
			if (tmp_dev == NULL) break;
			if (tmp_dev->bus_number != 0) {
				usbi_dbg("got bus number from ancestor #%d", i);
				parent_dev->bus_number = tmp_dev->bus_number;
				break;
			}
		}
	}
	if (parent_dev->bus_number == 0) {
		usbi_err(ctx, "program assertion failed: unable to find ancestor bus number for '%s'", device_id);
		return LIBUSB_ERROR_NOT_FOUND;
	}
	dev->bus_number = parent_dev->bus_number;
	priv->port = port_number;
	priv->depth = parent_priv->depth + 1;
	priv->parent_dev = parent_dev;

	// If the device address is already set, we can stop here
	if (dev->device_address != 0) {
		return LIBUSB_SUCCESS;
	}
	memset(&conn_info, 0, sizeof(conn_info));
	if (priv->depth != 0) {	// Not a HCD hub
		handle = CreateFileA(parent_priv->path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED, NULL);
		if (handle == INVALID_HANDLE_VALUE) {
			usbi_warn(ctx, "could not open hub %s: %s", parent_priv->path, windows_error_str(0));
			return LIBUSB_ERROR_ACCESS;
		}
		size = sizeof(conn_info);
		conn_info.ConnectionIndex = (ULONG)port_number;
		if (!DeviceIoControl(handle, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &conn_info, size,
			&conn_info, size, &size, NULL)) {
			usbi_warn(ctx, "could not get node connection information for device '%s': %s",
				device_id, windows_error_str(0));
			safe_closehandle(handle);
			return LIBUSB_ERROR_NO_DEVICE;
		}
		if (conn_info.ConnectionStatus == NoDeviceConnected) {
			usbi_err(ctx, "device '%s' is no longer connected!", device_id);
			safe_closehandle(handle);
			return LIBUSB_ERROR_NO_DEVICE;
		}
		memcpy(&priv->dev_descriptor, &(conn_info.DeviceDescriptor), sizeof(USB_DEVICE_DESCRIPTOR));
		dev->num_configurations = priv->dev_descriptor.bNumConfigurations;
		priv->active_config = conn_info.CurrentConfigurationValue;
		usbi_dbg("found %d configurations (active conf: %d)", dev->num_configurations, priv->active_config);
		// If we can't read the config descriptors, just set the number of confs to zero
		if (cache_config_descriptors(dev, handle, device_id) != LIBUSB_SUCCESS) {
			dev->num_configurations = 0;
			priv->dev_descriptor.bNumConfigurations = 0;
		}
		safe_closehandle(handle);

		if (conn_info.DeviceAddress > UINT8_MAX) {
			usbi_err(ctx, "program assertion failed: device address overflow");