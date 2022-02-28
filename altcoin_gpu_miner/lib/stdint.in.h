/* Copyright (C) 2001-2002, 2004-2011 Free Software Foundation, Inc.
   Written by Paul Eggert, Bruno Haible, Sam Steingold, Peter Burwood.
   This file is part of gnulib.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/*
 * ISO C 99 <stdint.h> for platforms that lack it.
 * <http://www.opengroup.org/susv3xbd/stdint.h.html>
 */

#ifndef _@GUARD_PREFIX@_STDINT_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

/* When including a system file that in turn includes <inttypes.h>,
   use the system <inttypes.h>, not our substitute.  This avoids
   problems with (for example) VMS, whose <sys/bitypes.h> includes
   <inttypes.h>.  */
#define _GL_JUST_INCLUDE_SYSTEM_INTTYPES_H

/* Get those types that are already defined in other system include
   files, so that we can "#define int8_t signed char" below without
   worrying about a later system include file containing a "typedef
   signed char int8_t;" that will get messed up by our macro.  Our
   macros should all be consistent with the system versions, except
   for the "fast" types and macros, which we recommend against using
   in public interfaces due to compiler differences.  */

#if @HAVE_STDINT_H@
# if defined __sgi && ! defined __c99
   /* Bypass IRIX's <stdint.h> if in C89 mode, since it merely annoys users
      with "This header file is to be used only for c99 mode compilations"
      diagnostics.  */
#  define __STDINT_H__
# endif
  /* Other systems may have an incomplete or buggy <stdint.h>.
     Include it before <inttypes.h>, since any "#include <stdint.h>"
     in <inttypes.h> would reinclude us, skipping our contents because
     _@GUARD_PREFIX@_STDINT_H is defined.
     The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_STDINT_H@
#endif

#if ! defined _@GUARD_PREFIX@_STDINT_H && ! defined _GL_JUST_INCLUDE_SYSTEM_STDINT_H
#define _@GUARD_PREFIX@_STDINT_H

/* <sys/types.h> defines some of the stdint.h types as well, on glibc,
   IRIX 6.5, and OpenBSD 3.8 (via <machine/types.h>).
   AIX 5.2 <sys/types.h> isn't needed and causes troubles.
   MacOS X 10.4.6 <sys/types.h> includes <stdint.h> (which is us), but
   relies on the system <stdint.h> definitions, so include
   <sys/types.h> after @NEXT_STDINT_H@.  */
#if @HAVE_SYS_TYPES_H@ && ! defined _AIX
# include <sys/types.h>
#endif

/* Get LONG_MIN, LONG_MAX, ULONG_MAX.  */
#include <limits.h>

#if @HAVE_INTTYPES_H@
  /* In OpenBSD 3.8, <inttypes.h> includes <machine/types.h>, which defines
     int{8,16,32,64}_t, uint{8,16,32,64}_t and __BIT_TYPES_DEFINED__.
     <inttypes.h> also defines intptr_t and uintptr_t.  */
# include <inttypes.h>
#elif @HAVE_SYS_INTTYPES_H@
  /* Solaris 7 <sys/inttypes.h> has the types except the *_fast*_t types, and
     the macros except for *_FAST*_*, INTPTR_MIN, PTRDIFF_MIN, PTRDIFF_MAX.  */
# include <sys/inttypes.h>
#endif

#if @HAVE_SYS_BITYPES_H@ && ! defined __BIT_TYPES_DEFINED__
  /* Linux libc4 >= 4.6.7 and libc5 have a <sys/bitypes.h> that defines
     int{8,16,32,64}_t and __BIT_TYPES_DEFINED__.  In libc5 >= 5.2.2 it is
     included by <sys/types.h>.  */
# include <sys/bitypes.h>
#endif

#undef _GL_JUST_INCLUDE_SYSTEM_INTTYPES_H

/* Minimum and maximum values for an integer type under the usual assumption.
   Return an unspecified value if BITS == 0, adding a check to pacify
   picky compilers.  */

#define _STDINT_MIN(signed, bits, zero) \
  ((signed) ? (- ((zero) + 1) << ((bits) ? (bits) - 1 : 0)) : (zero))

#define _STDINT_MAX(signed, bits, zero) \
  ((signed) \
   ? ~ _STDINT_MIN (signed, bits, zero) \
   : /* The expression for the unsigned case.  The subtraction of (signed) \
        is a nop in the unsigned case and avoids "signed integer overflow" \
        warnings in the signed case.  */ \
     ((((zero) + 1) << ((bits) ? (bits) - 1 - (signed) : 0)) - 1) * 2 + 1)

#if !GNULIB_defined_stdint_types

/* 7.18.1.1. Exact-width integer types */

/* Here we assume a standard architecture where the hardware integer
   types have 8, 16, 32, optionally 64 bits.  */

#undef int8_t
#undef uint8_t
typedef signed char gl_int8_t;
typedef unsigned char gl_uint8_t;
#define int8_t gl_int8_t
#define uint8_t gl_uint8_t

#undef int16_t
#undef uint16_t
typedef short int gl_int16_t;
typedef unsigned short int gl_uint16_t;
#define int16_t gl_int16_t
#define uint16_t gl_uint16_t

#undef int32_t
#undef uint32_t
typedef int gl_int32_t;
typedef unsigned int gl_uint32_t;
#define int32_t gl_int32_t
#define uint32_t gl_uint32_t

/* If the system defines INT64_MAX, assume int64_t works.  That way,
   if the underlying platform defines int64_t to be a 64-bit long long
   int, the code below won't mistakenly define it to be a 64-bit long
   int, which would mess up C++ name mangling.  We must use #ifdef
   rather than #if, to avoid an error with HP-UX 10.20 cc.  */

#ifdef INT64_MAX
# define GL_INT64_T
#else
/* Do not undefine int64_t if gnulib is not being used with 64-bit
   types, since otherwise it breaks platforms like Tandem/NSK.  */
# if LONG_MAX >> 31 >> 31 == 1
#  undef int64_t
typedef long int gl_int64_t;
#  define int64_t gl_int64_t
#  define GL_INT64_T
# elif defined _MSC_VER
#  undef int64_t
typedef __int64 gl_int64_t;
#  define int64_t gl_int64_t
#  define GL_INT64_T
# elif @HAVE_LONG_LONG_INT@
#  undef int64_t
typedef long long int gl_int64_t;
#  define int64_t gl_int64_t
#  define GL_INT64_T
# endif
#endif

#ifdef UINT64_MAX
# define GL_UINT64_T
#else
# if ULONG_MAX >> 31 >> 31 >> 1 == 1
#  undef uint64_t
typedef unsigned long int gl_uint64_t;
#  define uint64_t gl_uint64_t
#  define GL_UINT64_T
# elif defined _MSC_VER
#  undef uint64_t
typedef unsigned __int64 gl_uint64_t;
#  define uint64_t gl_uint64_t
#  define GL_UINT64_T
# elif @HAVE_UNSIGNED_LONG_LONG_INT@
#  undef uint64_t
typedef unsigned long long int gl_uint64_t;
#  define uint64_t gl_uint64_t
#  define GL_UINT64_T
# endif
#endif

/* Avoid collision with Solaris 2.5.1 <pthread.h> etc.  */
#define _UINT8_T
#define _UINT32_T
#define _UINT64_T


/* 7.18.1.2. Minimum-width integer types */

/* Here we assume a standard architecture where the hardware integer
   types have 8, 16, 32, optionally 64 bits. Therefore the leastN_t types
   are the same as the corresponding N_t types.  */

#undef int_least8_t
#undef uint_least8_t
#undef int_least16_t
#undef uint_least16_t
#undef int_least32_t
#undef uint_least32_t
#undef int_least64_t
#undef uint_least64_t
#define int_least8_t int8_t
#define uint_least8_t uint8_t
#define int_least16_t int16_t
#define uint_least16_t uint16_t
#define int_least32_t int32_t
#define uint_least32_t uint32_t
#ifdef GL_INT64_T
# define int_least64_t int64_t
#endif
#ifdef GL_UINT64_T
# define uint_least64_t u