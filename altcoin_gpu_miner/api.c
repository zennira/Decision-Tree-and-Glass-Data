
/*
 * Copyright 2011-2013 Andrew Smith
 * Copyright 2011-2013 Con Kolivas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 *
 * Note: the code always includes GPU support even if there are no GPUs
 *	this simplifies handling multiple other device code being included
 *	depending on compile options
 */
#define _MEMORY_DEBUG_MASTER 1

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

#include "compat.h"
#include "miner.h"
#include "util.h"

#if defined(USE_BFLSC) || defined(USE_AVALON) || defined(USE_HASHFAST) || defined(USE_BITFURY) || defined(USE_KLONDIKE) || defined(USE_KNC)
#define HAVE_AN_ASIC 1
#endif

#if defined(USE_BITFORCE) || defined(USE_ICARUS) || defined(USE_MODMINER)
#define HAVE_AN_FPGA 1
#endif

// Big enough for largest API request
//  though a PC with 100s of PGAs may exceed the size ...
//  data is truncated at the end of the last record that fits
//	but still closed correctly for JSON
// Current code assumes it can socket send this size + JSON_CLOSE + JSON_END
#define SOCKBUFSIZ	65432

// BUFSIZ varies on Windows and Linux
#define TMPBUFSIZ	8192

// Number of requests to queue - normally would be small
// However lots of PGA's may mean more
#define QUEUE	100

#if defined WIN32
static char WSAbuf[1024];

struct WSAERRORS {
	int id;
	char *code;
} WSAErrors[] = {
	{ 0,			"No error" },
	{ WSAEINTR,		"Interrupted system call" },
	{ WSAEBADF,		"Bad file number" },
	{ WSAEACCES,		"Permission denied" },
	{ WSAEFAULT,		"Bad address" },
	{ WSAEINVAL,		"Invalid argument" },
	{ WSAEMFILE,		"Too many open sockets" },
	{ WSAEWOULDBLOCK,	"Operation would block" },
	{ WSAEINPROGRESS,	"Operation now in progress" },
	{ WSAEALREADY,		"Operation already in progress" },
	{ WSAENOTSOCK,		"Socket operation on non-socket" },
	{ WSAEDESTADDRREQ,	"Destination address required" },
	{ WSAEMSGSIZE,		"Message too long" },
	{ WSAEPROTOTYPE,	"Protocol wrong type for socket" },
	{ WSAENOPROTOOPT,	"Bad protocol option" },
	{ WSAEPROTONOSUPPORT,	"Protocol not supported" },
	{ WSAESOCKTNOSUPPORT,	"Socket type not supported" },
	{ WSAEOPNOTSUPP,	"Operation not supported on socket" },
	{ WSAEPFNOSUPPORT,	"Protocol family not supported" },
	{ WSAEAFNOSUPPORT,	"Address family not supported" },
	{ WSAEADDRINUSE,	"Address already in use" },
	{ WSAEADDRNOTAVAIL,	"Can't assign requested address" },
	{ WSAENETDOWN,		"Network is down" },
	{ WSAENETUNREACH,	"Network is unreachable" },
	{ WSAENETRESET,		"Net connection reset" },
	{ WSAECONNABORTED,	"Software caused connection abort" },
	{ WSAECONNRESET,	"Connection reset by peer" },
	{ WSAENOBUFS,		"No buffer space available" },
	{ WSAEISCONN,		"Socket is already connected" },
	{ WSAENOTCONN,		"Socket is not connected" },
	{ WSAESHUTDOWN,		"Can't send after socket shutdown" },
	{ WSAETOOMANYREFS,	"Too many references, can't splice" },
	{ WSAETIMEDOUT,		"Connection timed out" },
	{ WSAECONNREFUSED,	"Connection refused" },
	{ WSAELOOP,		"Too many levels of symbolic links" },
	{ WSAENAMETOOLONG,	"File name too long" },
	{ WSAEHOSTDOWN,		"Host is down" },
	{ WSAEHOSTUNREACH,	"No route to host" },
	{ WSAENOTEMPTY,		"Directory not empty" },
	{ WSAEPROCLIM,		"Too many processes" },
	{ WSAEUSERS,		"Too many users" },
	{ WSAEDQUOT,		"Disc quota exceeded" },
	{ WSAESTALE,		"Stale NFS file handle" },
	{ WSAEREMOTE,		"Too many levels of remote in path" },
	{ WSASYSNOTREADY,	"Network system is unavailable" },
	{ WSAVERNOTSUPPORTED,	"Winsock version out of range" },
	{ WSANOTINITIALISED,	"WSAStartup not yet called" },
	{ WSAEDISCON,		"Graceful shutdown in progress" },
	{ WSAHOST_NOT_FOUND,	"Host not found" },
	{ WSANO_DATA,		"No host data of that type was found" },
	{ -1,			"Unknown error code" }
};

char *WSAErrorMsg(void) {
	int i;
	int id = WSAGetLastError();

	/* Assume none of them are actually -1 */
	for (i = 0; WSAErrors[i].id != -1; i++)
		if (WSAErrors[i].id == id)
			break;

	sprintf(WSAbuf, "Socket Error: (%d) %s", id, WSAErrors[i].code);

	return &(WSAbuf[0]);
}
#endif

static const char *UNAVAILABLE = " - API will not be available";
static const char *MUNAVAILABLE = " - API multicast listener will not be available";

static const char *BLANK = "";
static const char *COMMA = ",";
#define COMSTR ","
static const char SEPARATOR = '|';
#define SEPSTR "|"
static const char GPUSEP = ',';

static const char *APIVERSION = "1.32";
static const char *DEAD = "Dead";
#if defined(HAVE_OPENCL) || defined(HAVE_AN_FPGA) || defined(HAVE_AN_ASIC)
static const char *SICK = "Sick";
static const char *NOSTART = "NoStart";
static const char *INIT = "Initialising";
#endif
static const char *DISABLED = "Disabled";
static const char *ALIVE = "Alive";
static const char *REJECTING = "Rejecting";
static const char *UNKNOWN = "Unknown";
#define _DYNAMIC "D"
#ifdef HAVE_OPENCL
static const char *DYNAMIC = _DYNAMIC;
#endif

static __maybe_unused const char *NONE = "None";

static const char *YES = "Y";
static const char *NO = "N";
static const char *NULLSTR = "(null)";

static const char *TRUESTR = "true";
static const char *FALSESTR = "false";

#ifdef USE_SCRYPT
static const char *SCRYPTSTR = "scrypt";
#endif
static const char *SHA256STR = "sha256";

static const char *DEVICECODE = ""
#ifdef USE_AVALON
			"AVA "
#endif
#ifdef USE_BFLSC
			"BAS "
#endif
#ifdef USE_BITFORCE
			"BFL "
#endif
#ifdef USE_BITFURY
			"BFU "
#endif
#ifdef HAVE_OPENCL
			"GPU "
#endif
#ifdef USE_HASHFAST
			"HFA "
#endif
#ifdef USE_ICARUS
			"ICA "
#endif
#ifdef USE_MODMINER
			"MMQ "
#endif
#ifdef USE_KNC
			"KnC "
#endif
#ifdef USE_MODMINER
			"MMQ "
#endif
			"";

static const char *OSINFO =
#if defined(__linux)
			"Linux";
#else
#if defined(__APPLE__)
			"Apple";
#else
#if defined (WIN32)
			"Windows";
#else
#if defined(unix)
			"Unix";
#else
			"Unknown";
#endif
#endif
#endif
#endif

#define _DEVS		"DEVS"
#define _POOLS		"POOLS"
#define _SUMMARY	"SUMMARY"
#define _STATUS		"STATUS"
#define _VERSION	"VERSION"
#define _MINECONFIG	"CONFIG"
#define _GPU		"GPU"

#ifdef HAVE_AN_FPGA
#define _PGA		"PGA"
#endif

#ifdef HAVE_AN_ASIC
#define _ASC		"ASC"
#endif

#define _GPUS		"GPUS"
#define _PGAS		"PGAS"
#define _ASCS		"ASCS"
#define _NOTIFY		"NOTIFY"
#define _DEVDETAILS	"DEVDETAILS"
#define _BYE		"BYE"
#define _RESTART	"RESTART"
#define _MINESTATS	"STATS"
#define _CHECK		"CHECK"
#define _MINECOIN	"COIN"
#define _DEBUGSET	"DEBUG"
#define _SETCONFIG	"SETCONFIG"
#define _USBSTATS	"USBSTATS"

static const char ISJSON = '{';
#define JSON0		"{"
#define JSON1		"\""
#define JSON2		"\":["
#define JSON3		"]"
#define JSON4		",\"id\":1"
// If anyone cares, id=0 for truncated output
#define JSON4_TRUNCATED	",\"id\":0"
#define JSON5		"}"

#define JSON_START	JSON0
#define JSON_DEVS	JSON1 _DEVS JSON2
#define JSON_POOLS	JSON1 _POOLS JSON2
#define JSON_SUMMARY	JSON1 _SUMMARY JSON2
#define JSON_STATUS	JSON1 _STATUS JSON2
#define JSON_VERSION	JSON1 _VERSION JSON2
#define JSON_MINECONFIG	JSON1 _MINECONFIG JSON2
#define JSON_GPU	JSON1 _GPU JSON2

#ifdef HAVE_AN_FPGA
#define JSON_PGA	JSON1 _PGA JSON2
#endif

#ifdef HAVE_AN_ASIC
#define JSON_ASC	JSON1 _ASC JSON2
#endif

#define JSON_GPUS	JSON1 _GPUS JSON2
#define JSON_PGAS	JSON1 _PGAS JSON2
#define JSON_ASCS	JSON1 _ASCS JSON2
#define JSON_NOTIFY	JSON1 _NOTIFY JSON2
#define JSON_DEVDETAILS	JSON1 _DEVDETAILS JSON2
#define JSON_BYE	JSON1 _BYE JSON1
#define JSON_RESTART	JSON1 _RESTART JSON1
#define JSON_CLOSE	JSON3
#define JSON_MINESTATS	JSON1 _MINESTATS JSON2
#define JSON_CHECK	JSON1 _CHECK JSON2
#define JSON_MINECOIN	JSON1 _MINECOIN JSON2
#define JSON_DEBUGSET	JSON1 _DEBUGSET JSON2
#define JSON_SETCONFIG	JSON1 _SETCONFIG JSON2
#define JSON_USBSTATS	JSON1 _USBSTATS JSON2
#define JSON_END	JSON4 JSON5
#define JSON_END_TRUNCATED	JSON4_TRUNCATED JSON5

static const char *JSON_COMMAND = "command";
static const char *JSON_PARAMETER = "parameter";

#define MSG_INVGPU 1
#define MSG_ALRENA 2
#define MSG_ALRDIS 3
#define MSG_GPUMRE 4
#define MSG_GPUREN 5
#define MSG_GPUNON 6
#define MSG_POOL 7
#define MSG_NOPOOL 8
#define MSG_DEVS 9
#define MSG_NODEVS 10
#define MSG_SUMM 11
#define MSG_GPUDIS 12
#define MSG_GPUREI 13
#define MSG_INVCMD 14
#define MSG_MISID 15
#define MSG_GPUDEV 17

#define MSG_NUMGPU 20

#define MSG_VERSION 22
#define MSG_INVJSON 23
#define MSG_MISCMD 24
#define MSG_MISPID 25
#define MSG_INVPID 26
#define MSG_SWITCHP 27
#define MSG_MISVAL 28
#define MSG_NOADL 29
#define MSG_NOGPUADL 30
#define MSG_INVINT 31
#define MSG_GPUINT 32
#define MSG_MINECONFIG 33
#define MSG_GPUMERR 34
#define MSG_GPUMEM 35
#define MSG_GPUEERR 36
#define MSG_GPUENG 37
#define MSG_GPUVERR 38
#define MSG_GPUVDDC 39
#define MSG_GPUFERR 40
#define MSG_GPUFAN 41
#define MSG_MISFN 42
#define MSG_BADFN 43
#define MSG_SAVED 44
#define MSG_ACCDENY 45
#define MSG_ACCOK 46
#define MSG_ENAPOOL 47
#define MSG_DISPOOL 48
#define MSG_ALRENAP 49
#define MSG_ALRDISP 50
#define MSG_DISLASTP 51
#define MSG_MISPDP 52
#define MSG_INVPDP 53
#define MSG_TOOMANYP 54
#define MSG_ADDPOOL 55

#ifdef HAVE_AN_FPGA
#define MSG_PGANON 56
#define MSG_PGADEV 57
#define MSG_INVPGA 58
#endif

#define MSG_NUMPGA 59
#define MSG_NOTIFY 60

#ifdef HAVE_AN_FPGA
#define MSG_PGALRENA 61
#define MSG_PGALRDIS 62
#define MSG_PGAENA 63
#define MSG_PGADIS 64
#define MSG_PGAUNW 65
#endif

#define MSG_REMLASTP 66
#define MSG_ACTPOOL 67
#define MSG_REMPOOL 68
#define MSG_DEVDETAILS 69
#define MSG_MINESTATS 70
#define MSG_MISCHK 71
#define MSG_CHECK 72
#define MSG_POOLPRIO 73
#define MSG_DUPPID 74
#define MSG_MISBOOL 75
#define MSG_INVBOOL 76
#define MSG_FOO 77
#define MSG_MINECOIN 78
#define MSG_DEBUGSET 79
#define MSG_PGAIDENT 80
#define MSG_PGANOID 81
#define MSG_SETCONFIG 82
#define MSG_UNKCON 83
#define MSG_INVNUM 84
#define MSG_CONPAR 85
#define MSG_CONVAL 86
#define MSG_USBSTA 87
#define MSG_NOUSTA 88

#ifdef HAVE_AN_FPGA
#define MSG_MISPGAOPT 89
#define MSG_PGANOSET 90
#define MSG_PGAHELP 91
#define MSG_PGASETOK 92
#define MSG_PGASETERR 93
#endif

#define MSG_ZERMIS 94
#define MSG_ZERINV 95
#define MSG_ZERSUM 96
#define MSG_ZERNOSUM 97
#define MSG_PGAUSBNODEV 98
#define MSG_INVHPLG 99
#define MSG_HOTPLUG 100
#define MSG_DISHPLG 101
#define MSG_NOHPLG 102
#define MSG_MISHPLG 103

#define MSG_NUMASC 104
#ifdef HAVE_AN_ASIC
#define MSG_ASCNON 105
#define MSG_ASCDEV 106
#define MSG_INVASC 107
#define MSG_ASCLRENA 108
#define MSG_ASCLRDIS 109
#define MSG_ASCENA 110
#define MSG_ASCDIS 111
#define MSG_ASCUNW 112
#define MSG_ASCIDENT 113
#define MSG_ASCNOID 114
#endif
#define MSG_ASCUSBNODEV 115

#ifdef HAVE_AN_ASIC
#define MSG_MISASCOPT 116
#define MSG_ASCNOSET 117
#define MSG_ASCHELP 118
#define MSG_ASCSETOK 119
#define MSG_ASCSETERR 120
#endif

#define MSG_INVNEG 121
#define MSG_SETQUOTA 122
#define MSG_LOCKOK 123
#define MSG_LOCKDIS 124

enum code_severity {
	SEVERITY_ERR,
	SEVERITY_WARN,
	SEVERITY_INFO,
	SEVERITY_SUCC,
	SEVERITY_FAIL
};

enum code_parameters {
	PARAM_GPU,
	PARAM_PGA,
	PARAM_ASC,
	PARAM_PID,
	PARAM_GPUMAX,
	PARAM_PGAMAX,
	PARAM_ASCMAX,
	PARAM_PMAX,
	PARAM_POOLMAX,

// Single generic case: have the code resolve it - see below
	PARAM_DMAX,

	PARAM_CMD,
	PARAM_POOL,
	PARAM_STR,
	PARAM_BOTH,
	PARAM_BOOL,
	PARAM_SET,
	PARAM_INT,
	PARAM_NONE
};

struct CODES {
	const enum code_severity severity;
	const int code;
	const enum code_parameters params;
	const char *description;
} codes[] = {
#ifdef HAVE_OPENCL
 { SEVERITY_ERR,   MSG_INVGPU,	PARAM_GPUMAX,	"Invalid GPU id %d - range is 0 - %d" },
 { SEVERITY_INFO,  MSG_ALRENA,	PARAM_GPU,	"GPU %d already enabled" },
 { SEVERITY_INFO,  MSG_ALRDIS,	PARAM_GPU,	"GPU %d already disabled" },
 { SEVERITY_WARN,  MSG_GPUMRE,	PARAM_GPU,	"GPU %d must be restarted first" },
 { SEVERITY_INFO,  MSG_GPUREN,	PARAM_GPU,	"GPU %d sent enable message" },
#endif
 { SEVERITY_ERR,   MSG_GPUNON,	PARAM_NONE,	"No GPUs" },
 { SEVERITY_SUCC,  MSG_POOL,	PARAM_PMAX,	"%d Pool(s)" },
 { SEVERITY_ERR,   MSG_NOPOOL,	PARAM_NONE,	"No pools" },

 { SEVERITY_SUCC,  MSG_DEVS,	PARAM_DMAX,
#ifdef HAVE_OPENCL
		 	 	 	 	"%d GPU(s)"
#endif
#if defined(HAVE_AN_ASIC) && defined(HAVE_OPENCL)
						" - "
#endif
#ifdef HAVE_AN_ASIC
						"%d ASC(s)"
#endif
#if defined(HAVE_AN_FPGA) && (defined(HAVE_OPENCL) || defined(HAVE_AN_ASIC))
						" - "
#endif
#ifdef HAVE_AN_FPGA
						"%d PGA(s)"
#endif
#if (defined(HAVE_OPENCL) || defined(HAVE_AN_ASIC) || defined(HAVE_AN_FPGA))
						" - "
#endif
 },

 { SEVERITY_ERR,   MSG_NODEVS,	PARAM_NONE,	"No GPUs"
#ifdef HAVE_AN_ASIC
						"/ASCs"
#endif
#ifdef HAVE_AN_FPGA
						"/PGAs"
#endif
 },

 { SEVERITY_SUCC,  MSG_SUMM,	PARAM_NONE,	"Summary" },
#ifdef HAVE_OPENCL
 { SEVERITY_INFO,  MSG_GPUDIS,	PARAM_GPU,	"GPU %d set disable flag" },
 { SEVERITY_INFO,  MSG_GPUREI,	PARAM_GPU,	"GPU %d restart attempted" },
#endif
 { SEVERITY_ERR,   MSG_INVCMD,	PARAM_NONE,	"Invalid command" },
 { SEVERITY_ERR,   MSG_MISID,	PARAM_NONE,	"Missing device id parameter" },
#ifdef HAVE_OPENCL
 { SEVERITY_SUCC,  MSG_GPUDEV,	PARAM_GPU,	"GPU%d" },
#endif
#ifdef HAVE_AN_FPGA
 { SEVERITY_ERR,   MSG_PGANON,	PARAM_NONE,	"No PGAs" },
 { SEVERITY_SUCC,  MSG_PGADEV,	PARAM_PGA,	"PGA%d" },
 { SEVERITY_ERR,   MSG_INVPGA,	PARAM_PGAMAX,	"Invalid PGA id %d - range is 0 - %d" },
 { SEVERITY_INFO,  MSG_PGALRENA,PARAM_PGA,	"PGA %d already enabled" },
 { SEVERITY_INFO,  MSG_PGALRDIS,PARAM_PGA,	"PGA %d already disabled" },
 { SEVERITY_INFO,  MSG_PGAENA,	PARAM_PGA,	"PGA %d sent enable message" },
 { SEVERITY_INFO,  MSG_PGADIS,	PARAM_PGA,	"PGA %d set disable flag" },
 { SEVERITY_ERR,   MSG_PGAUNW,	PARAM_PGA,	"PGA %d is not flagged WELL, cannot enable" },
#endif
 { SEVERITY_SUCC,  MSG_NUMGPU,	PARAM_NONE,	"GPU count" },
 { SEVERITY_SUCC,  MSG_NUMPGA,	PARAM_NONE,	"PGA count" },
 { SEVERITY_SUCC,  MSG_NUMASC,	PARAM_NONE,	"ASC count" },
 { SEVERITY_SUCC,  MSG_VERSION,	PARAM_NONE,	"CGMiner versions" },
 { SEVERITY_ERR,   MSG_INVJSON,	PARAM_NONE,	"Invalid JSON" },
 { SEVERITY_ERR,   MSG_MISCMD,	PARAM_CMD,	"Missing JSON '%s'" },
 { SEVERITY_ERR,   MSG_MISPID,	PARAM_NONE,	"Missing pool id parameter" },
 { SEVERITY_ERR,   MSG_INVPID,	PARAM_POOLMAX,	"Invalid pool id %d - range is 0 - %d" },
 { SEVERITY_SUCC,  MSG_SWITCHP,	PARAM_POOL,	"Switching to pool %d:'%s'" },
 { SEVERITY_ERR,   MSG_MISVAL,	PARAM_NONE,	"Missing comma after GPU number" },
 { SEVERITY_ERR,   MSG_NOADL,	PARAM_NONE,	"ADL is not available" },
 { SEVERITY_ERR,   MSG_NOGPUADL,PARAM_GPU,	"GPU %d does not have ADL" },
 { SEVERITY_ERR,   MSG_INVINT,	PARAM_STR,	"Invalid intensity (%s) - must be '" _DYNAMIC  "' or range " MIN_SHA_INTENSITY_STR " - " MAX_SCRYPT_INTENSITY_STR },
 { SEVERITY_INFO,  MSG_GPUINT,	PARAM_BOTH,	"GPU %d set new intensity to %s" },
 { SEVERITY_SUCC,  MSG_MINECONFIG,PARAM_NONE,	"CGMiner config" },
#ifdef HAVE_OPENCL
 { SEVERITY_ERR,   MSG_GPUMERR,	PARAM_BOTH,	"Setting GPU %d memoryclock to (%s) reported failure" },
 { SEVERITY_SUCC,  MSG_GPUMEM,	PARAM_BOTH,	"Setting GPU %d memoryclock to (%s) reported success" },
 { SEVERITY_ERR,   MSG_GPUEERR,	PARAM_BOTH,	"Setting GPU %d clock to (%s) reported failure" },
 { SEVERITY_SUCC,  MSG_GPUENG,	PARAM_BOTH,	"Setting GPU %d clock to (%s) reported success" },
 { SEVERITY_ERR,   MSG_GPUVERR,	PARAM_BOTH,	"Setting GPU %d vddc to (%s) reported failure" },
 { SEVERITY_SUCC,  MSG_GPUVDDC,	PARAM_BOTH,	"Setting GPU %d vddc to (%s) reported success" },
 { SEVERITY_ERR,   MSG_GPUFERR,	PARAM_BOTH,	"Setting GPU %d fan to (%s) reported failure" },
 { SEVERITY_SUCC,  MSG_GPUFAN,	PARAM_BOTH,	"Setting GPU %d fan to (%s) reported success" },
#endif
 { SEVERITY_ERR,   MSG_MISFN,	PARAM_NONE,	"Missing save filename parameter" },
 { SEVERITY_ERR,   MSG_BADFN,	PARAM_STR,	"Can't open or create save file '%s'" },
 { SEVERITY_SUCC,  MSG_SAVED,	PARAM_STR,	"Configuration saved to file '%s'" },
 { SEVERITY_ERR,   MSG_ACCDENY,	PARAM_STR,	"Access denied to '%s' command" },
 { SEVERITY_SUCC,  MSG_ACCOK,	PARAM_NONE,	"Privileged access OK" },
 { SEVERITY_SUCC,  MSG_ENAPOOL,	PARAM_POOL,	"Enabling pool %d:'%s'" },
 { SEVERITY_SUCC,  MSG_POOLPRIO,PARAM_NONE,	"Changed pool priorities" },
 { SEVERITY_ERR,   MSG_DUPPID,	PARAM_PID,	"Duplicate pool specified %d" },
 { SEVERITY_SUCC,  MSG_DISPOOL,	PARAM_POOL,	"Disabling pool %d:'%s'" },
 { SEVERITY_INFO,  MSG_ALRENAP,	PARAM_POOL,	"Pool %d:'%s' already enabled" },
 { SEVERITY_INFO,  MSG_ALRDISP,	PARAM_POOL,	"Pool %d:'%s' already disabled" },
 { SEVERITY_ERR,   MSG_DISLASTP,PARAM_POOL,	"Cannot disable last active pool %d:'%s'" },
 { SEVERITY_ERR,   MSG_MISPDP,	PARAM_NONE,	"Missing addpool details" },
 { SEVERITY_ERR,   MSG_INVPDP,	PARAM_STR,	"Invalid addpool details '%s'" },
 { SEVERITY_ERR,   MSG_TOOMANYP,PARAM_NONE,	"Reached maximum number of pools (%d)" },
 { SEVERITY_SUCC,  MSG_ADDPOOL,	PARAM_STR,	"Added pool '%s'" },
 { SEVERITY_ERR,   MSG_REMLASTP,PARAM_POOL,	"Cannot remove last pool %d:'%s'" },
 { SEVERITY_ERR,   MSG_ACTPOOL, PARAM_POOL,	"Cannot remove active pool %d:'%s'" },
 { SEVERITY_SUCC,  MSG_REMPOOL, PARAM_BOTH,	"Removed pool %d:'%s'" },
 { SEVERITY_SUCC,  MSG_NOTIFY,	PARAM_NONE,	"Notify" },
 { SEVERITY_SUCC,  MSG_DEVDETAILS,PARAM_NONE,	"Device Details" },
 { SEVERITY_SUCC,  MSG_MINESTATS,PARAM_NONE,	"CGMiner stats" },
 { SEVERITY_ERR,   MSG_MISCHK,	PARAM_NONE,	"Missing check cmd" },
 { SEVERITY_SUCC,  MSG_CHECK,	PARAM_NONE,	"Check command" },
 { SEVERITY_ERR,   MSG_MISBOOL,	PARAM_NONE,	"Missing parameter: true/false" },
 { SEVERITY_ERR,   MSG_INVBOOL,	PARAM_NONE,	"Invalid parameter should be true or false" },
 { SEVERITY_SUCC,  MSG_FOO,	PARAM_BOOL,	"Failover-Only set to %s" },
 { SEVERITY_SUCC,  MSG_MINECOIN,PARAM_NONE,	"CGMiner coin" },
 { SEVERITY_SUCC,  MSG_DEBUGSET,PARAM_NONE,	"Debug settings" },
#ifdef HAVE_AN_FPGA
 { SEVERITY_SUCC,  MSG_PGAIDENT,PARAM_PGA,	"Identify command sent to PGA%d" },
 { SEVERITY_WARN,  MSG_PGANOID,	PARAM_PGA,	"PGA%d does not support identify" },
#endif
 { SEVERITY_SUCC,  MSG_SETCONFIG,PARAM_SET,	"Set config '%s' to %d" },
 { SEVERITY_ERR,   MSG_UNKCON,	PARAM_STR,	"Unknown config '%s'" },
 { SEVERITY_ERR,   MSG_INVNUM,	PARAM_BOTH,	"Invalid number (%d) for '%s' range is 0-9999" },
 { SEVERITY_ERR,   MSG_INVNEG,	PARAM_BOTH,	"Invalid negative number (%d) for '%s'" },
 { SEVERITY_SUCC,  MSG_SETQUOTA,PARAM_SET,	"Set pool '%s' to quota %d'" },
 { SEVERITY_ERR,   MSG_CONPAR,	PARAM_NONE,	"Missing config parameters 'name,N'" },
 { SEVERITY_ERR,   MSG_CONVAL,	PARAM_STR,	"Missing config value N for '%s,N'" },
 { SEVERITY_SUCC,  MSG_USBSTA,	PARAM_NONE,	"USB Statistics" },
 { SEVERITY_INFO,  MSG_NOUSTA,	PARAM_NONE,	"No USB Statistics" },
#ifdef HAVE_AN_FPGA
 { SEVERITY_ERR,   MSG_MISPGAOPT, PARAM_NONE,	"Missing option after PGA number" },
 { SEVERITY_WARN,  MSG_PGANOSET, PARAM_PGA,	"PGA %d does not support pgaset" },
 { SEVERITY_INFO,  MSG_PGAHELP, PARAM_BOTH,	"PGA %d set help: %s" },
 { SEVERITY_SUCC,  MSG_PGASETOK, PARAM_BOTH,	"PGA %d set OK" },
 { SEVERITY_ERR,   MSG_PGASETERR, PARAM_BOTH,	"PGA %d set failed: %s" },
#endif
 { SEVERITY_ERR,   MSG_ZERMIS,	PARAM_NONE,	"Missing zero parameters" },
 { SEVERITY_ERR,   MSG_ZERINV,	PARAM_STR,	"Invalid zero parameter '%s'" },
 { SEVERITY_SUCC,  MSG_ZERSUM,	PARAM_STR,	"Zeroed %s stats with summary" },
 { SEVERITY_SUCC,  MSG_ZERNOSUM, PARAM_STR,	"Zeroed %s stats without summary" },
#ifdef USE_USBUTILS
 { SEVERITY_ERR,   MSG_PGAUSBNODEV, PARAM_PGA,	"PGA%d has no device" },
 { SEVERITY_ERR,   MSG_ASCUSBNODEV, PARAM_PGA,	"ASC%d has no device" },
#endif
 { SEVERITY_ERR,   MSG_INVHPLG,	PARAM_STR,	"Invalid value for hotplug (%s) must be 0..9999" },
 { SEVERITY_SUCC,  MSG_HOTPLUG,	PARAM_INT,	"Hotplug check set to %ds" },
 { SEVERITY_SUCC,  MSG_DISHPLG,	PARAM_NONE,	"Hotplug disabled" },
 { SEVERITY_WARN,  MSG_NOHPLG,	PARAM_NONE,	"Hotplug is not available" },
 { SEVERITY_ERR,   MSG_MISHPLG,	PARAM_NONE,	"Missing hotplug parameter" },
#ifdef HAVE_AN_ASIC
 { SEVERITY_ERR,   MSG_ASCNON,	PARAM_NONE,	"No ASCs" },
 { SEVERITY_SUCC,  MSG_ASCDEV,	PARAM_ASC,	"ASC%d" },
 { SEVERITY_ERR,   MSG_INVASC,	PARAM_ASCMAX,	"Invalid ASC id %d - range is 0 - %d" },
 { SEVERITY_INFO,  MSG_ASCLRENA,PARAM_ASC,	"ASC %d already enabled" },
 { SEVERITY_INFO,  MSG_ASCLRDIS,PARAM_ASC,	"ASC %d already disabled" },
 { SEVERITY_INFO,  MSG_ASCENA,	PARAM_ASC,	"ASC %d sent enable message" },
 { SEVERITY_INFO,  MSG_ASCDIS,	PARAM_ASC,	"ASC %d set disable flag" },
 { SEVERITY_ERR,   MSG_ASCUNW,	PARAM_ASC,	"ASC %d is not flagged WELL, cannot enable" },
 { SEVERITY_SUCC,  MSG_ASCIDENT,PARAM_ASC,	"Identify command sent to ASC%d" },
 { SEVERITY_WARN,  MSG_ASCNOID,	PARAM_ASC,	"ASC%d does not support identify" },
 { SEVERITY_ERR,   MSG_MISASCOPT, PARAM_NONE,	"Missing option after ASC number" },
 { SEVERITY_WARN,  MSG_ASCNOSET, PARAM_ASC,	"ASC %d does not support ascset" },
 { SEVERITY_INFO,  MSG_ASCHELP, PARAM_BOTH,	"ASC %d set help: %s" },
 { SEVERITY_SUCC,  MSG_ASCSETOK, PARAM_BOTH,	"ASC %d set OK" },
 { SEVERITY_ERR,   MSG_ASCSETERR, PARAM_BOTH,	"ASC %d set failed: %s" },
#endif
 { SEVERITY_SUCC,  MSG_LOCKOK,	PARAM_NONE,	"Lock stats created" },
 { SEVERITY_WARN,  MSG_LOCKDIS,	PARAM_NONE,	"Lock stats not enabled" },
 { SEVERITY_FAIL, 0, 0, NULL }
};

static const char *localaddr = "127.0.0.1";

static int my_thr_id = 0;
static bool bye;

// Used to control quit restart access to shutdown variables
static pthread_mutex_t quit_restart_lock;

static bool do_a_quit;
static bool do_a_restart;

static time_t when = 0;	// when the request occurred

struct IP4ACCESS {
	in_addr_t ip;
	in_addr_t mask;
	char group;
};

#define GROUP(g) (toupper(g))
#define PRIVGROUP GROUP('W')
#define NOPRIVGROUP GROUP('R')
#define ISPRIVGROUP(g) (GROUP(g) == PRIVGROUP)
#define GROUPOFFSET(g) (GROUP(g) - GROUP('A'))
#define VALIDGROUP(g) (GROUP(g) >= GROUP('A') && GROUP(g) <= GROUP('Z'))
#define COMMANDS(g) (apigroups[GROUPOFFSET(g)].commands)
#define DEFINEDGROUP(g) (ISPRIVGROUP(g) || COMMANDS(g) != NULL)

struct APIGROUPS {
	// This becomes a string like: "|cmd1|cmd2|cmd3|" so it's quick to search
	char *commands;
} apigroups['Z' - 'A' + 1]; // only A=0 to Z=25 (R: noprivs, W: allprivs)

static struct IP4ACCESS *ipaccess = NULL;
static int ips = 0;

struct io_data {
	size_t siz;
	char *ptr;
	char *cur;
	bool sock;
	bool full;
	bool close;
};

struct io_list {
	struct io_data *io_data;
	struct io_list *prev;
	struct io_list *next;
};

static struct io_list *io_head = NULL;

#define io_new(init) _io_new(init, false)
#define sock_io_new() _io_new(SOCKBUFSIZ, true)

static void io_reinit(struct io_data *io_data)
{
	io_data->cur = io_data->ptr;
	*(io_data->ptr) = '\0';
	io_data->full = false;
	io_data->close = false;
}

static struct io_data *_io_new(size_t initial, bool socket_buf)
{
	struct io_data *io_data;
	struct io_list *io_list;

	io_data = malloc(sizeof(*io_data));
	io_data->ptr = malloc(initial);
	io_data->siz = initial;
	io_data->sock = socket_buf;
	io_reinit(io_data);

	io_list = malloc(sizeof(*io_list));

	io_list->io_data = io_data;

	if (io_head) {
		io_list->next = io_head;
		io_list->prev = io_head->prev;
		io_list->next->prev = io_list;
		io_list->prev->next = io_list;
	} else {
		io_list->prev = io_list;
		io_list->next = io_list;
		io_head = io_list;
	}

	return io_data;
}

static bool io_add(struct io_data *io_data, char *buf)
{
	size_t len, dif, tot;

	if (io_data->full)
		return false;

	len = strlen(buf);
	dif = io_data->cur - io_data->ptr;
	tot = len + 1 + dif;

	if (tot > io_data->siz) {
		size_t new = io_data->siz * 2;

		if (new < tot)
			new = tot * 2;

		if (io_data->sock) {
			if (new > SOCKBUFSIZ) {
				if (tot > SOCKBUFSIZ) {
					io_data->full = true;
					return false;
				}

				new = SOCKBUFSIZ;
			}
		}

		io_data->ptr = realloc(io_data->ptr, new);
		io_data->cur = io_data->ptr + dif;
		io_data->siz = new;
	}

	memcpy(io_data->cur, buf, len + 1);
	io_data->cur += len;

	return true;
}

static bool io_put(struct io_data *io_data, char *buf)
{
	io_reinit(io_data);
	return io_add(io_data, buf);
}

static void io_close(struct io_data *io_data)
{
	io_data->close = true;
}

static void io_free()
{
	struct io_list *io_list, *io_next;

	if (io_head) {
		io_list = io_head;
		do {
			io_next = io_list->next;

			free(io_list->io_data);
			free(io_list);

			io_list = io_next;
		} while (io_list != io_head);

		io_head = NULL;
	}
}

// This is only called when expected to be needed (rarely)
// i.e. strings outside of the codes control (input from the user)
static char *escape_string(char *str, bool isjson)
{
	char *buf, *ptr;
	int count;

	count = 0;
	for (ptr = str; *ptr; ptr++) {
		switch (*ptr) {
			case ',':
			case '|':
			case '=':
				if (!isjson)
					count++;
				break;
			case '"':
				if (isjson)
					count++;
				break;
			case '\\':
				count++;
				break;
		}
	}

	if (count == 0)
		return str;

	buf = malloc(strlen(str) + count + 1);
	if (unlikely(!buf))
		quit(1, "Failed to malloc escape buf");

	ptr = buf;
	while (*str)
		switch (*str) {
			case ',':
			case '|':
			case '=':
				if (!isjson)
					*(ptr++) = '\\';
				*(ptr++) = *(str++);
				break;
			case '"':
				if (isjson)
					*(ptr++) = '\\';
				*(ptr++) = *(str++);
				break;
			case '\\':
				*(ptr++) = '\\';
				*(ptr++) = *(str++);
				break;
			default:
				*(ptr++) = *(str++);
				break;
		}

	*ptr = '\0';

	return buf;
}

static struct api_data *api_add_extra(struct api_data *root, struct api_data *extra)
{
	struct api_data *tmp;

	if (root) {
		if (extra) {
			// extra tail
			tmp = extra->prev;

			// extra prev = root tail
			extra->prev = root->prev;

			// root tail next = extra
			root->prev->next = extra;

			// extra tail next = root
			tmp->next = root;

			// root prev = extra tail
			root->prev = tmp;
		}
	} else
		root = extra;

	return root;
}

static struct api_data *api_add_data_full(struct api_data *root, char *name, enum api_data_type type, void *data, bool copy_data)
{
	struct api_data *api_data;

	api_data = (struct api_data *)malloc(sizeof(struct api_data));

	api_data->name = strdup(name);
	api_data->type = type;

	if (root == NULL) {
		root = api_data;
		root->prev = root;
		root->next = root;
	}
	else {
		api_data->prev = root->prev;
		root->prev = api_data;
		api_data->next = root;
		api_data->prev->next = api_data;
	}

	api_data->data_was_malloc = copy_data;

	// Avoid crashing on bad data
	if (data == NULL) {
		api_data->type = type = API_CONST;
		data = (void *)NULLSTR;
		api_data->data_was_malloc = copy_data = false;
	}

	if (!copy_data)
		api_data->data = data;
	else
		switch(type) {
			case API_ESCAPE:
			case API_STRING:
			case API_CONST:
				api_data->data = (void *)malloc(strlen((char *)data) + 1);
				strcpy((char*)(api_data->data), (char *)data);
				break;
			case API_UINT8:
				/* Most OSs won't really alloc less than 4 */
				api_data->data = malloc(4);
				*(uint8_t *)api_data->data = *(uint8_t *)data;
				break;
			case API_UINT16:
				/* Most OSs won't really alloc less than 4 */
				api_data->data = malloc(4);
				*(uint16_t *)api_data->data = *(uint16_t *)data;
				break;
			case API_INT:
				api_data->data = (void *)malloc(sizeof(int));
				*((int *)(api_data->data)) = *((int *)data);
				break;
			case API_UINT:
				api_data->data = (void *)malloc(sizeof(unsigned int));
				*((unsigned int *)(api_data->data)) = *((unsigned int *)data);
				break;
			case API_UINT32:
				api_data->data = (void *)malloc(sizeof(uint32_t));
				*((uint32_t *)(api_data->data)) = *((uint32_t *)data);
				break;
			case API_UINT64:
				api_data->data = (void *)malloc(sizeof(uint64_t));
				*((uint64_t *)(api_data->data)) = *((uint64_t *)data);
				break;
			case API_DOUBLE:
			case API_ELAPSED:
			case API_MHS:
			case API_MHTOTAL:
			case API_UTILITY:
			case API_FREQ:
			case API_HS:
			case API_DIFF:
			case API_PERCENT:
				api_data->data = (void *)malloc(sizeof(double));
				*((double *)(api_data->data)) = *((double *)data);
				break;
			case API_BOOL:
				api_data->data = (void *)malloc(sizeof(bool));
				*((bool *)(api_data->data)) = *((bool *)data);
				break;
			case API_TIMEVAL:
				api_data->data = (void *)malloc(sizeof(struct timeval));
				memcpy(api_data->data, data, sizeof(struct timeval));
				break;
			case API_TIME:
				api_data->data = (void *)malloc(sizeof(time_t));
				*(time_t *)(api_data->data) = *((time_t *)data);
				break;
			case API_VOLTS:
			case API_TEMP:
				api_data->data = (void *)malloc(sizeof(float));
				*((float *)(api_data->data)) = *((float *)data);
				break;
			default:
				applog(LOG_ERR, "API: unknown1 data type %d ignored", type);
				api_data->type = API_STRING;
				api_data->data_was_malloc = false;
				api_data->data = (void *)UNKNOWN;
				break;
		}

	return root;
}

struct api_data *api_add_escape(struct api_data *root, char *name, char *data, bool copy_data)
{
	return api_add_data_full(root, name, API_ESCAPE, (void *)data, copy_data);
}

struct api_data *api_add_string(struct api_data *root, char *name, char *data, bool copy_data)
{
	return api_add_data_full(root, name, API_STRING, (void *)data, copy_data);
}

struct api_data *api_add_const(struct api_data *root, char *name, const char *data, bool copy_data)
{
	return api_add_data_full(root, name, API_CONST, (void *)data, copy_data);
}

struct api_data *api_add_uint8(struct api_data *root, char *name, uint8_t *data, bool copy_data)
{
	return api_add_data_full(root, name, API_UINT8, (void *)data, copy_data);
}

struct api_data *api_add_uint16(struct api_data *root, char *name, uint16_t *data, bool copy_data)
{
	return api_add_data_full(root, name, API_UINT16, (void *)data, copy_data);
}

struct api_data *api_add_int(struct api_data *root, char *name, int *data, bool copy_data)
{
	return api_add_data_full(root, name, API_INT, (void *)data, copy_data);
}

struct api_data *api_add_uint(struct api_data *root, char *name, unsigned int *data, bool copy_data)
{
	return api_add_data_full(root, name, API_UINT, (void *)data, copy_data);
}

struct api_data *api_add_uint32(struct api_data *root, char *name, uint32_t *data, bool copy_data)
{
	return api_add_data_full(root, name, API_UINT32, (void *)data, copy_data);
}

struct api_data *api_add_uint64(struct api_data *root, char *name, uint64_t *data, bool copy_data)
{
	return api_add_data_full(root, name, API_UINT64, (void *)data, copy_data);
}

struct api_data *api_add_double(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_DOUBLE, (void *)data, copy_data);
}

struct api_data *api_add_elapsed(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_ELAPSED, (void *)data, copy_data);
}

struct api_data *api_add_bool(struct api_data *root, char *name, bool *data, bool copy_data)
{
	return api_add_data_full(root, name, API_BOOL, (void *)data, copy_data);
}

struct api_data *api_add_timeval(struct api_data *root, char *name, struct timeval *data, bool copy_data)
{
	return api_add_data_full(root, name, API_TIMEVAL, (void *)data, copy_data);
}

struct api_data *api_add_time(struct api_data *root, char *name, time_t *data, bool copy_data)
{
	return api_add_data_full(root, name, API_TIME, (void *)data, copy_data);
}

struct api_data *api_add_mhs(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_MHS, (void *)data, copy_data);
}

struct api_data *api_add_mhtotal(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_MHTOTAL, (void *)data, copy_data);
}

struct api_data *api_add_temp(struct api_data *root, char *name, float *data, bool copy_data)
{
	return api_add_data_full(root, name, API_TEMP, (void *)data, copy_data);
}

struct api_data *api_add_utility(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_UTILITY, (void *)data, copy_data);
}

struct api_data *api_add_freq(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_FREQ, (void *)data, copy_data);
}

struct api_data *api_add_volts(struct api_data *root, char *name, float *data, bool copy_data)
{
	return api_add_data_full(root, name, API_VOLTS, (void *)data, copy_data);
}

struct api_data *api_add_hs(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_HS, (void *)data, copy_data);
}

struct api_data *api_add_diff(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_DIFF, (void *)data, copy_data);
}

struct api_data *api_add_percent(struct api_data *root, char *name, double *data, bool copy_data)
{
	return api_add_data_full(root, name, API_PERCENT, (void *)data, copy_data);
}

static struct api_data *print_data(struct api_data *root, char *buf, bool isjson, bool precom)
{
	struct api_data *tmp;
	bool first = true;
	char *original, *escape;
	char *quote;

	*buf = '\0';

	if (precom) {
		*(buf++) = *COMMA;
		*buf = '\0';
	}

	if (isjson) {
		strcpy(buf, JSON0);
		buf = strchr(buf, '\0');
		quote = JSON1;
	} else
		quote = (char *)BLANK;

	while (root) {
		if (!first)
			*(buf++) = *COMMA;
		else
			first = false;

		sprintf(buf, "%s%s%s%s", quote, root->name, quote, isjson ? ":" : "=");

		buf = strchr(buf, '\0');

		switch(root->type) {
			case API_STRING:
			case API_CONST:
				sprintf(buf, "%s%s%s", quote, (char *)(root->data), quote);
				break;
			case API_ESCAPE:
				original = (char *)(root->data);
				escape = escape_string((char *)(root->data), isjson);
				sprintf(buf, "%s%s%s", quote, escape, quote);
				if (escape != original)
					free(escape);
				break;
			case API_UINT8:
				sprintf(buf, "%u", *(uint8_t *)root->data);
				break;
			case API_UINT16:
				sprintf(buf, "%u", *(uint16_t *)root->data);
				break;
			case API_INT:
				sprintf(buf, "%d", *((int *)(root->data)));
				break;
			case API_UINT:
				sprintf(buf, "%u", *((unsigned int *)(root->data)));
				break;
			case API_UINT32:
				sprintf(buf, "%"PRIu32, *((uint32_t *)(root->data)));
				break;
			case API_UINT64:
				sprintf(buf, "%"PRIu64, *((uint64_t *)(root->data)));
				break;
			case API_TIME:
				sprintf(buf, "%lu", *((unsigned long *)(root->data)));
				break;
			case API_DOUBLE:
				sprintf(buf, "%f", *((double *)(root->data)));
				break;
			case API_ELAPSED:
				sprintf(buf, "%.0f", *((double *)(root->data)));
				break;
			case API_UTILITY:
			case API_FREQ:
			case API_MHS:
				sprintf(buf, "%.2f", *((double *)(root->data)));
				break;
			case API_VOLTS:
				sprintf(buf, "%.3f", *((float *)(root->data)));
				break;
			case API_MHTOTAL:
				sprintf(buf, "%.4f", *((double *)(root->data)));
				break;
			case API_HS:
				sprintf(buf, "%.15f", *((double *)(root->data)));
				break;
			case API_DIFF:
				sprintf(buf, "%.8f", *((double *)(root->data)));
				break;
			case API_BOOL:
				sprintf(buf, "%s", *((bool *)(root->data)) ? TRUESTR : FALSESTR);
				break;
			case API_TIMEVAL:
				sprintf(buf, "%ld.%06ld",
					((struct timeval *)(root->data))->tv_sec,
					((struct timeval *)(root->data))->tv_usec);
				break;
			case API_TEMP:
				sprintf(buf, "%.2f", *((float *)(root->data)));
				break;
			case API_PERCENT:
				sprintf(buf, "%.4f", *((double *)(root->data)) * 100.0);
				break;
			default:
				applog(LOG_ERR, "API: unknown2 data type %d ignored", root->type);
				sprintf(buf, "%s%s%s", quote, UNKNOWN, quote);
				break;
		}

		buf = strchr(buf, '\0');

		free(root->name);
		if (root->data_was_malloc)
			free(root->data);

		if (root->next == root) {
			free(root);
			root = NULL;
		} else {
			tmp = root;
			root = tmp->next;
			root->prev = tmp->prev;
			root->prev->next = root;
			free(tmp);
		}
	}

	strcpy(buf, isjson ? JSON5 : SEPSTR);

	return root;
}

#define DRIVER_COUNT_DRV(X) if (devices[i]->drv->drv_id == DRIVER_##X) \
	count++;

#ifdef HAVE_AN_ASIC
static int numascs(void)
{
	int count = 0;
	int i;

	rd_lock(&devices_lock);
	for (i = 0; i < total_devices; i++) {
		ASIC_PARSE_COMMANDS(DRIVER_COUNT_DRV)
	}
	rd_unlock(&devices_lock);
	return count;
}

static int ascdevice(int ascid)
{
	int count = 0;
	int i;

	rd_lock(&devices_lock);
	for (i = 0; i < total_devices; i++) {
		ASIC_PARSE_COMMANDS(DRIVER_COUNT_DRV)
		if (count == (ascid + 1))
			goto foundit;
	}

	rd_unlock(&devices_lock);
	return -1;

foundit:

	rd_unlock(&devices_lock);
	return i;
}
#endif

#ifdef HAVE_AN_FPGA
static int numpgas(void)
{
	int count = 0;
	int i;

	rd_lock(&devices_lock);
	for (i = 0; i < total_devices; i++) {
		FPGA_PARSE_COMMANDS(DRIVER_COUNT_DRV)
	}
	rd_unlock(&devices_lock);
	return count;
}

static int pgadevice(int pgaid)
{
	int count = 0;
	int i;

	rd_lock(&devices_lock);
	for (i = 0; i < total_devices; i++) {
		FPGA_PARSE_COMMANDS(DRIVER_COUNT_DRV)
		if (count == (pgaid + 1))
			goto foundit;
	}

	rd_unlock(&devices_lock);
	return -1;

foundit:

	rd_unlock(&devices_lock);
	return i;
}
#endif

// All replies (except BYE and RESTART) start with a message
//  thus for JSON, message() inserts JSON_START at the front
//  and send_result() adds JSON_END at the end
static void message(struct io_data *io_data, int messageid, int paramid, char *param2, bool isjson)
{
	struct api_data *root = NULL;
	char buf[TMPBUFSIZ];
	char buf2[TMPBUFSIZ];
	char severity[2];
#ifdef HAVE_AN_ASIC
	int asc;
#endif
#ifdef HAVE_AN_FPGA
	int pga;
#endif
	int i;

	io_reinit(io_data);

	if (isjson)
		io_put(io_data, JSON_START JSON_STATUS);

	for (i = 0; codes[i].severity != SEVERITY_FAIL; i++) {
		if (codes[i].code == messageid) {
			switch (codes[i].severity) {
				case SEVERITY_WARN:
					severity[0] = 'W';
					break;
				case SEVERITY_INFO:
					severity[0] = 'I';
					break;
				case SEVERITY_SUCC:
					severity[0] = 'S';
					break;
				case SEVERITY_ERR:
				default:
					severity[0] = 'E';
					break;
			}
			severity[1] = '\0';

			switch(codes[i].params) {
				case PARAM_GPU:
				case PARAM_PGA:
				case PARAM_ASC:
				case PARAM_PID:
				case PARAM_INT:
					sprintf(buf, codes[i].description, paramid);
					break;
				case PARAM_POOL:
					sprintf(buf, codes[i].description, paramid, pools[paramid]->rpc_url);
					break;
#ifdef HAVE_OPENCL
				case PARAM_GPUMAX:
					sprintf(buf, codes[i].description, paramid, nDevs - 1);
					break;
#endif
#ifdef HAVE_AN_FPGA
				case PARAM_PGAMAX:
					pga = numpgas();
					sprintf(buf, codes[i].description, paramid, pga - 1);
					break;
#endif
#ifdef HAVE_AN_ASIC
				case PARAM_ASCMAX:
					asc = numascs();
					sprintf(buf, codes[i].description, paramid, asc - 1);
					break;
#endif
				case PARAM_PMAX:
					sprintf(buf, codes[i].description, total_pools);
					break;
				case PARAM_POOLMAX:
					sprintf(buf, codes[i].description, paramid, total_pools - 1);
					break;
				case PARAM_DMAX:
#ifdef HAVE_AN_ASIC
					asc = numascs();
#endif
#ifdef HAVE_AN_FPGA
					pga = numpgas();
#endif

					sprintf(buf, codes[i].description
#ifdef HAVE_OPENCL
						, nDevs
#endif
#ifdef HAVE_AN_ASIC
						, asc
#endif
#ifdef HAVE_AN_FPGA
						, pga
#endif
						);
					break;
				case PARAM_CMD:
					sprintf(buf, codes[i].description, JSON_COMMAND);
					break;
				case PARAM_STR:
					sprintf(buf, codes[i].description, param2);
					break;
				case PARAM_BOTH:
					sprintf(buf, codes[i].description, paramid, param2);
					break;
				case PARAM_BOOL:
					sprintf(buf, codes[i].description, paramid ? TRUESTR : FALSESTR);
					break;
				case PARAM_SET:
					sprintf(buf, codes[i].description, param2, paramid);
					break;
				case PARAM_NONE:
				default:
					strcpy(buf, codes[i].description);
			}

			root = api_add_string(root, _STATUS, severity, false);
			root = api_add_time(root, "When", &when, false);
			root = api_add_int(root, "Code", &messageid, false);
			root = api_add_escape(root, "Msg", buf, false);
			root = api_add_escape(root, "Description", opt_api_description, false);

			root = print_data(root, buf2, isjson, false);
			io_add(io_data, buf2);
			if (isjson)
				io_add(io_data, JSON_CLOSE);
			return;
		}
	}

	root = api_add_string(root, _STATUS, "F", false);
	root = api_add_time(root, "When", &when, false);
	int id = -1;
	root = api_add_int(root, "Code", &id, false);
	sprintf(buf, "%d", messageid);
	root = api_add_escape(root, "Msg", buf, false);
	root = api_add_escape(root, "Description", opt_api_description, false);

	root = print_data(root, buf2, isjson, false);
	io_add(io_data, buf2);
	if (isjson)
		io_add(io_data, JSON_CLOSE);
}

#if LOCK_TRACKING

#define LOCK_FMT_FFL " - called from %s %s():%d"

#define LOCKMSG(fmt, ...)	fprintf(stderr, "APILOCK: " fmt "\n", ##__VA_ARGS__)
#define LOCKMSGMORE(fmt, ...)	fprintf(stderr, "          " fmt "\n", ##__VA_ARGS__)
#define LOCKMSGFFL(fmt, ...) fprintf(stderr, "APILOCK: " fmt LOCK_FMT_FFL "\n", ##__VA_ARGS__, file, func, linenum)
#define LOCKMSGFLUSH() fflush(stderr)

typedef struct lockstat {
	uint64_t lock_id;
	const char *file;
	const char *func;
	int linenum;
	struct timeval tv;
} LOCKSTAT;

typedef struct lockline {
	struct lockline *prev;
	struct lockstat *stat;
	struct lockline *next;
} LOCKLINE;

typedef struct lockinfo {
	void *lock;
	enum cglock_typ typ;
	const char *file;
	const char *func;
	int linenum;
	uint64_t gets;
	uint64_t gots;
	uint64_t tries;
	uint64_t dids;
	uint64_t didnts; // should be tries - dids
	uint64_t unlocks;
	LOCKSTAT lastgot;
	LOCKLINE *lockgets;
	LOCKLINE *locktries;
} LOCKINFO;

typedef struct locklist {
	LOCKINFO *info;
	struct locklist *next;
} LOCKLIST;

static uint64_t lock_id = 1;

static LOCKLIST *lockhead;

static void lockmsgnow()
{
	struct timeval now;
	struct tm *tm;
	time_t dt;

	cgtime(&now);

	dt = now.tv_sec;
	tm = localtime(&dt);

	LOCKMSG("%d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);
}

static LOCKLIST *newlock(void *lock, enum cglock_typ typ, const char *file, const char *func, const int linenum)
{
	LOCKLIST *list;
