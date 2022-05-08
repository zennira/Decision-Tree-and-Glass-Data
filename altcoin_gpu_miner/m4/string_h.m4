# Configure a GNU-like replacement for <string.h>.

# Copyright (C) 2007-2011 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# serial 20

# Written by Paul Eggert.

AC_DEFUN([gl_HEADER_STRING_H],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_HEADER_STRING_H_BODY])
])

AC_DEFUN([gl_HEADER_STRING_H_BODY],
[
  AC_REQUIRE([AC_C_RESTRICT])
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  gl_NEXT_HEADERS([string.h])

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use, and which is not
  dnl guaranteed by C89.
  gl_WARN_ON_USE_PREPARE([[#include <string.h>
    ]],
    [memmem mempcpy memrchr rawmemchr stpcpy stpncpy strchrnul strdup
     strncat strndup strnlen strpbrk strsep strcasestr strtok_r strerror_r
     strsignal strverscmp])
])

AC_DEFUN([gl_STRING_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_HEADER_STRING_H_DEFAULTS],
[
  GNULIB_MEMCHR=0;      AC_SUBST([GNULIB_MEMCHR])
  GNULIB_MEMMEM=0;      AC_SUBST([GNULIB_MEMMEM])
  GNULIB_MEMPCPY=0;     AC_SUBST([GNULIB_MEMPCPY])
  GNULIB_MEMRCHR=0;     AC_SUBST([GNULIB_MEMRCHR])
  GNULIB_RAWMEMCHR=0;   AC_SUBST([GNULIB_RAWMEMCHR])
  GNULIB_STPCPY=0;      AC_SUBST([GNULIB_STPCPY])
  GNULIB_STPNCPY=0;     AC_SUBST([GNULIB_STPNCPY])
  GNULIB_STRCHRNUL=0;   AC_SUBST([GNULIB_STRCHRNUL])
  GNULIB_STRDUP=0;      AC_SUBST([GNULIB_STRDUP])
  GNULIB_STRNCAT=0;     AC_SUBST([GNULIB_STRNCAT])
  GNULIB_STRNDUP=0;     AC_SUBST([GNULIB_STRNDUP])
  GNULIB_STRNLEN=0;     AC_SUBST([GNULIB_STRNLEN])
  GNULIB_STRPBRK=0;     AC_SUBST([GNULIB_STRPBRK])
  GNULIB_STRSEP=0;      AC_SUBST([GNULIB_STRSEP])
  GNULIB_STRSTR=0;      AC_SUBST([GNULIB_STRSTR])
  GNULIB_STRCASESTR=0;  AC_SUBST([GNULIB_STRCASESTR])
  GNULIB_STRTOK_R=0;    AC_SUBST([GNULIB_STRTOK_R])
  GNULIB_MBSLEN=0;      AC_SUBST([GNULIB_MBSLEN])
  GNULIB_MBSNLEN=0;     AC_SUBST([GNULIB_MBSNLEN])
  GNULIB_MBSCHR=0;      AC_SUBST([GNULIB_MBSCHR])
  GNULIB_MBSRCHR=0;     AC_SUBST([GNULIB_MBSRCHR])
  GNULIB_MBSSTR=0;      AC_SUBST([GNULIB_MBSSTR])
  GNULIB_MBSCASECMP=0;  AC_SUBST([GNULIB_MBSCASECMP])
  GNULIB_MBSNCASECMP=0; AC_SUBST([GNULIB_MBSNCASECMP])
  GNULIB_MBSPCASECMP=0; AC_SUBST([GNULIB_MBSPCASECMP])
  GNULIB_MBSCASESTR=0;  AC_SUBST([GNULIB_MBSCASESTR])
  GNULIB_MBSCSPN=0;     AC_SUBST([GNULIB_MBSCSPN])
  GNULIB_MBSPBRK=0;     AC_SUBST([GNULIB_MBSPBRK])
  GNULIB_MBSSPN=0;      AC_SUBST([GNULIB_MBSSPN])
  GNULIB_MBSSEP=0;      AC_SUBST([GNULIB_MBSSEP])
  GNULIB_MBSTOK_R=0;    AC_SUBST([GNULIB_MBSTOK_R])
  GNULIB_STRERROR=0;    AC_SUBST([GNULIB_STRERROR])
  GNULIB_STRERROR_R=0;  AC_SUBST([GNULIB_STRERROR_R])
  GNULIB_STRSIGNAL=0;   AC_SUBST([GNULIB_STRSIGNAL])
  GNULIB_STRVERSCMP=0;  AC_SUBST([GNULIB_STRVERSCMP])
 