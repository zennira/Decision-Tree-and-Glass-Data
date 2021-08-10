
dnl These m4 macros are whitespace sensitive and break if moved around much.
m4_define([LU_VERSION_H], m4_include([libusb/version.h]))
m4_define([LU_DEFINE_VERSION_ATOM],
	[m4_define([$1], m4_bregexp(LU_VERSION_H,
	[^#define\s*$1\s*\([0-9]*\).*], [\1]))])
m4_define([LU_DEFINE_VERSION_RC_ATOM],
	[m4_define([$1], m4_bregexp(LU_VERSION_H,
	[^#define\s*$1\s*"\(-rc[0-9]*\)".*], [\1]))])
dnl The m4_bregexp() returns (only) the numbers following the #define named
dnl in the first macro parameter. m4_define() then defines the name for use
dnl in AC_INIT().

LU_DEFINE_VERSION_ATOM([LIBUSB_MAJOR])
LU_DEFINE_VERSION_ATOM([LIBUSB_MINOR])
LU_DEFINE_VERSION_ATOM([LIBUSB_MICRO])
LU_DEFINE_VERSION_RC_ATOM([LIBUSB_RC])

AC_INIT([libusb], LIBUSB_MAJOR[.]LIBUSB_MINOR[.]LIBUSB_MICRO[]LIBUSB_RC, [libusb-devel@lists.sourceforge.net], [libusb], [http://www.libusb.org/])

# Library versioning
# These numbers should be tweaked on every release. Read carefully:
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# http://sourceware.org/autobook/autobook/autobook_91.html
lt_current="2"
lt_revision="0"
lt_age="0"
LTLDFLAGS="-version-info ${lt_current}:${lt_revision}:${lt_age}"

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_MAINTAINER_MODE

AC_CONFIG_SRCDIR([libusb/core.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREREQ([2.50])
AC_PROG_CC
AC_PROG_LIBTOOL
LT_LANG([Windows Resource])
AC_C_INLINE
AM_PROG_CC_C_O
AC_DEFINE([_GNU_SOURCE], 1, [Use GNU extensions])

LTLDFLAGS="${LTLDFLAGS} -no-undefined"

AC_MSG_CHECKING([operating system])
case $host in
*-linux*)
	AC_MSG_RESULT([Linux])
	backend="linux"
	;;
*-darwin*)
	AC_MSG_RESULT([Darwin/Mac OS X])
	backend="darwin"
	;;
*-openbsd*)
	AC_MSG_RESULT([OpenBSD])
	backend="openbsd"
	;;
*-netbsd*)
	AC_MSG_RESULT([NetBSD (using OpenBSD backend)])
	backend="openbsd"
	;;
*-mingw*)
	AC_MSG_RESULT([Windows])
	backend="windows"
	;;
*-cygwin*)
	AC_MSG_RESULT([Cygwin (using Windows backend)])
	backend="windows"
	threads="posix"
	;;
*)
	AC_MSG_ERROR([unsupported operating system])
esac
case $backend in
linux)
	AC_DEFINE(OS_LINUX, 1, [Linux backend])
	AC_SUBST(OS_LINUX)
	AC_CHECK_LIB(rt, clock_gettime, -pthread)
	AC_ARG_ENABLE([udev],
                      [AC_HELP_STRING([--with-udev], [use udev for device enumeration and hotplug support (recommended, default: yes)])],
                      [], [enable_udev="yes"])
        if test "x$enable_udev" = "xyes" ; then
                # system has udev. use it or fail!
                AC_CHECK_HEADERS([libudev.h],[],[AC_ERROR(["udev support requested but libudev not installed"])])
                AC_CHECK_LIB([udev], [udev_new], [], [AC_ERROR(["udev support requested but libudev not installed"])])
                AC_DEFINE(USE_UDEV, 1, [Use udev for device enumeration/hotplug])
        else
                AC_CHECK_HEADERS([linux/netlink.h linux/filter.h], [], [AC_ERROR(["Linux netlink headers not found"])])
        fi
        AC_SUBST(USE_UDEV)
	threads="posix"
	THREAD_CFLAGS="-pthread"
	LIBS="${LIBS} -pthread"
	AC_CHECK_HEADERS([poll.h])
	AC_DEFINE([POLL_NFDS_TYPE],[nfds_t],[type of second poll() argument])
	;;
darwin)
	AC_DEFINE(OS_DARWIN, 1, [Darwin backend])
	AC_SUBST(OS_DARWIN)
	threads="posix"
	LIBS="-lobjc -Wl,-framework,IOKit -Wl,-framework,CoreFoundation"
	LTLDFLAGS="${LTLDFLAGS} -Wl,-prebind"
	AC_CHECK_HEADERS([poll.h])
	AC_CHECK_TYPE([nfds_t],
		[AC_DEFINE([POLL_NFDS_TYPE],[nfds_t],[type of second poll() argument])],
		[AC_DEFINE([POLL_NFDS_TYPE],[unsigned int],[type of second poll() argument])],
		[#include <poll.h>])
	;;
openbsd)
	AC_DEFINE(OS_OPENBSD, 1, [OpenBSD backend])
	AC_SUBST(OS_OPENBSD)
	threads="posix"
	THREAD_CFLAGS="-pthread"
	LIBS="-pthread"
	AC_CHECK_HEADERS([poll.h])
	AC_DEFINE([POLL_NFDS_TYPE],[nfds_t],[type of second poll() argument])
	;;
windows)
	AC_DEFINE(OS_WINDOWS, 1, [Windows backend])
	AC_SUBST(OS_WINDOWS)
	LIBS=""
	LTLDFLAGS="${LTLDFLAGS} -avoid-version -Wl,--add-stdcall-alias"
	AC_DEFINE([POLL_NFDS_TYPE],[unsigned int],[type of second poll() argument])
	;;
esac
AC_SUBST(LIBS)

AM_CONDITIONAL(OS_LINUX, test "x$backend" = xlinux)
AM_CONDITIONAL(OS_DARWIN, test "x$backend" = xdarwin)
AM_CONDITIONAL(OS_OPENBSD, test "x$backend" = xopenbsd)
AM_CONDITIONAL(OS_WINDOWS, test "x$backend" = xwindows)
AM_CONDITIONAL(THREADS_POSIX, test "x$threads" = xposix)
AM_CONDITIONAL(USE_UDEV, test "x$enable_udev" = xyes)
if test "$threads" = posix; then
	AC_DEFINE(THREADS_POSIX, 1, [Use POSIX Threads])
fi

# timerfd
AC_CHECK_HEADER([sys/timerfd.h], [timerfd_h=1], [timerfd_h=0])
AC_ARG_ENABLE([timerfd],
	[AS_HELP_STRING([--enable-timerfd],
		[use timerfd for timing (default auto)])],
	[use_timerfd=$enableval], [use_timerfd='auto'])

if test "x$use_timerfd" = "xyes" -a "x$timerfd_h" = "x0"; then
	AC_MSG_ERROR([timerfd header not available; glibc 2.9+ required])
fi

AC_CHECK_DECL([TFD_NONBLOCK], [tfd_hdr_ok=yes], [tfd_hdr_ok=no], [#include <sys/timerfd.h>])
if test "x$use_timerfd" = "xyes" -a "x$tfd_hdr_ok" = "xno"; then
	AC_MSG_ERROR([timerfd header not usable; glibc 2.9+ required])
fi

AC_MSG_CHECKING([whether to use timerfd for timing])
if test "x$use_timerfd" = "xno"; then
	AC_MSG_RESULT([no (disabled by user)])
else