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
#if defined(_MSC_VER) && (_MSC_VER < 1600) && (!defined(_STDINT)) && (!defined(_STDINT