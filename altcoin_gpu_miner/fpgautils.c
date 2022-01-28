
/*
 * Copyright 2013 Con Kolivas <kernel@kolivas.org>
 * Copyright 2012 Luke Dashjr
 * Copyright 2012 Andrew Smith
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "config.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "miner.h"

#ifndef WIN32
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#else
#include <windows.h>
#include <io.h>
#endif

#ifdef HAVE_LIBUDEV
#include <libudev.h>
#include <sys/ioctl.h>
#endif

#include "elist.h"
#include "logging.h"
#include "miner.h"
#include "fpgautils.h"