/* Copyright (C) 1991-1994, 1996-1998, 2000, 2004, 2007-2011 Free Software
   Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* This particular implementation was written by Eric Blake, 2008.  */

#ifndef _LIBC
# include <config.h>
#endif

/* Specification of memmem.  */
#include <string.h>

#ifndef _LIBC
# define __builtin_expect(expr, val)   (expr)
#endif

#define RETURN_TYPE void *
#define AVAILABLE(h, h_l, j, n_l) ((j) <= (h_l) - (n_l))
#include "str-two-way.h"

/* Return the first occurrence of NEEDLE in HAYSTACK.  Return HAYSTACK
   if NEEDLE_LEN is 0, otherwise NULL if NEEDLE is not found in
   HAYSTACK.  */
void *
memmem (const void *haystack_start, size_t haystack_len,
        const void *needle_start, size_t needle_len)
{
  /* Abstract memory is considered to be an array of 'unsigned char' values,
     not an array of 'char' values.  See ISO C