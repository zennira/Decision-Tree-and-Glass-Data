# longlong.m4 serial 16
dnl Copyright (C) 1999-2007, 2009-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

# Define HAVE_LONG_LONG_INT if 'long long int' works.
# This fixes a bug in Autoconf 2.61, and can be faster
# than what's in Autoconf 2.62 through 2.68.

# Note: If the type 'long long int' exists but is only 32 bits large
# (as on some very old compilers), HAVE_LONG_LONG_INT will not be
# defined. In this case you can treat 'long long int' like 'long int'.

AC_DEFUN([AC_TYPE_LONG_LONG_INT],
[
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
  AC_CACHE_CHECK([for long long int], [ac_cv_type_long_long_int],
     [ac_cv_type_long_long_int=yes
      if test "x${ac_cv_prog_cc_c99-no}" = xno; then
        ac_cv_type_long_long_int=$ac_cv_type_unsigned_long_long_int
        if test $ac_cv_type_long_long_int = yes; then
          dnl Catch a bug in Tandem NonStop Kernel (OSS) cc -O circa 2004.
          dnl If cross compiling, assume the bug is not important, since
          dnl nobody cross compiles for this platform as far as we know.
          AC_RUN_IFELSE(
            [AC_LANG_PROGRAM(
               [[@%:@include <limits.h>
                 @%:@ifndef LLONG_MAX
                 @%:@ define HALF \
                          (1LL << (sizeof (long long int) * CHAR_BIT - 2))
                 @%:@ define LLONG_MAX (HALF - 1 + HALF)
                 @%:@endif]],
               [[long long int n = 1;
                 int i;
                 for (i = 0; ; i++)
                   {
                     long long int m = n << i;
                     if (m >> i != n)
                       return 1;
                     if (LLONG_MAX / 2 < m)
                       break;
                   }
                 return 0;]])],
            [],
            [ac_cv_type_long_long_int=no],
            [:])
        fi
      fi])
  if test $ac_cv_type_long_long_int = yes; then
    AC_DEFINE([HAVE_LONG_LONG_INT], [1],
      [Define to 1 if the system has the type `long long int'.])
  fi
])

# Define HAVE_UNSIGNED_LONG_LONG_INT if 'unsigned long long int' works.
# This fixes a bug in Autoconf 2.61, and can be faster
# than what's in Autoconf 2.62 through 2.68.

# Note: If the type 'unsigned long long in