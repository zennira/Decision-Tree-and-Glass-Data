

# libtool (GNU libtool) 2.4
# Written by Gordon Matzigkeit <gord@gnu.ai.mit.edu>, 1996

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005, 2006,
# 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.  There is NO
# warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# GNU Libtool is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# As a special exception to the GNU General Public License,
# if you distribute this file as part of a program or library that
# is built using GNU Libtool, you may include this file under the
# same distribution terms that you use for the rest of that program.
#
# GNU Libtool is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Libtool; see the file COPYING.  If not, a copy
# can be downloaded from http://www.gnu.org/licenses/gpl.html,
# or obtained by writing to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# Usage: $progname [OPTION]... [MODE-ARG]...
#
# Provide generalized library-building support services.
#
#       --config             show all configuration variables
#       --debug              enable verbose shell tracing
#   -n, --dry-run            display commands without modifying any files
#       --features           display basic configuration information and exit
#       --mode=MODE          use operation mode MODE
#       --preserve-dup-deps  don't remove duplicate dependency libraries
#       --quiet, --silent    don't print informational messages
#       --no-quiet, --no-silent
#                            print informational messages (default)
#       --tag=TAG            use configuration variables from tag TAG
#   -v, --verbose            print more informational messages than default
#       --no-verbose         don't print the extra informational messages
#       --version            print version information
#   -h, --help, --help-all   print short, long, or detailed help message
#
# MODE must be one of the following:
#
#         clean              remove files from the build directory
#         compile            compile a source file into a libtool object
#         execute            automatically set library path, then run a program
#         finish             complete the installation of libtool libraries
#         install            install libraries or executables
#         link               create a library or an executable
#         uninstall          remove libraries from an installed directory
#
# MODE-ARGS vary depending on the MODE.  When passed as first option,
# `--mode=MODE' may be abbreviated as `MODE' or a unique abbreviation of that.
# Try `$progname --help --mode=MODE' for a more detailed description of MODE.
#
# When reporting a bug, please describe a test case to reproduce it and
# include the following information:
#
#         host-triplet:	$host
#         shell:		$SHELL
#         compiler:		$LTCC
#         compiler flags:		$LTCFLAGS
#         linker:		$LD (gnu? $with_gnu_ld)
#         $progname:	(GNU libtool) 2.4
#         automake:	$automake_version
#         autoconf:	$autoconf_version
#
# Report bugs to <bug-libtool@gnu.org>.
# GNU libtool home page: <http://www.gnu.org/software/libtool/>.
# General help using GNU software: <http://www.gnu.org/gethelp/>.

PROGRAM=libtool
PACKAGE=libtool
VERSION=2.4
TIMESTAMP=""
package_revision=1.3294

# Be Bourne compatible
if test -n "${ZSH_VERSION+set}" && (emulate sh) >/dev/null 2>&1; then
  emulate sh
  NULLCMD=:
  # Zsh 3.x and 4.x performs word splitting on ${1+"$@"}, which
  # is contrary to our usage.  Disable this feature.
  alias -g '${1+"$@"}'='"$@"'
  setopt NO_GLOB_SUBST
else
  case `(set -o) 2>/dev/null` in *posix*) set -o posix;; esac
fi
BIN_SH=xpg4; export BIN_SH # for Tru64
DUALCASE=1; export DUALCASE # for MKS sh

# A function that is used when there is no print builtin or printf.
func_fallback_echo ()
{
  eval 'cat <<_LTECHO_EOF
$1
_LTECHO_EOF'
}

# NLS nuisances: We save the old values to restore during execute mode.
lt_user_locale=
lt_safe_locale=
for lt_var in LANG LANGUAGE LC_ALL LC_CTYPE LC_COLLATE LC_MESSAGES
do
  eval "if test \"\${$lt_var+set}\" = set; then
          save_$lt_var=\$$lt_var
          $lt_var=C
	  export $lt_var
	  lt_user_locale=\"$lt_var=\\\$save_\$lt_var; \$lt_user_locale\"
	  lt_safe_locale=\"$lt_var=C; \$lt_safe_locale\"
	fi"
done
LC_ALL=C
LANGUAGE=C
export LANGUAGE LC_ALL

$lt_unset CDPATH


# Work around backward compatibility issue on IRIX 6.5. On IRIX 6.4+, sh
# is ksh but when the shell is invoked as "sh" and the current value of
# the _XPG environment variable is not equal to 1 (one), the special
# positional parameter $0, within a function call, is the name of the
# function.
progpath="$0"



: ${CP="cp -f"}
test "${ECHO+set}" = set || ECHO=${as_echo-'printf %s\n'}
: ${EGREP="/bin/grep -E"}
: ${FGREP="/bin/grep -F"}
: ${GREP="/bin/grep"}
: ${LN_S="cp -p"}
: ${MAKE="make"}
: ${MKDIR="mkdir"}
: ${MV="mv -f"}
: ${RM="rm -f"}
: ${SED="/bin/sed"}
: ${SHELL="${CONFIG_SHELL-/bin/sh}"}
: ${Xsed="$SED -e 1s/^X//"}

# Global variables:
EXIT_SUCCESS=0
EXIT_FAILURE=1
EXIT_MISMATCH=63  # $? = 63 is used to indicate version mismatch to missing.
EXIT_SKIP=77	  # $? = 77 is used to indicate a skipped test to automake.

exit_status=$EXIT_SUCCESS

# Make sure IFS has a sensible default
lt_nl='
'
IFS=" 	$lt_nl"

dirname="s,/[^/]*$,,"
basename="s,^.*/,,"

# func_dirname file append nondir_replacement
# Compute the dirname of FILE.  If nonempty, add APPEND to the result,
# otherwise set result to NONDIR_REPLACEMENT.
func_dirname ()
{
    func_dirname_result=`$ECHO "${1}" | $SED "$dirname"`
    if test "X$func_dirname_result" = "X${1}"; then
      func_dirname_result="${3}"
    else
      func_dirname_result="$func_dirname_result${2}"
    fi
} # func_dirname may be replaced by extended shell implementation


# func_basename file
func_basename ()
{
    func_basename_result=`$ECHO "${1}" | $SED "$basename"`
} # func_basename may be replaced by extended shell implementation


# func_dirname_and_basename file append nondir_replacement
# perform func_basename and func_dirname in a single function
# call:
#   dirname:  Compute the dirname of FILE.  If nonempty,
#             add APPEND to the result, otherwise set result
#             to NONDIR_REPLACEMENT.
#             value returned in "$func_dirname_result"
#   basename: Compute filename of FILE.
#             value retuned in "$func_basename_result"
# Implementation must be kept synchronized with func_dirname
# and func_basename. For efficiency, we do not delegate to
# those functions but instead duplicate the functionality here.
func_dirname_and_basename ()
{
    # Extract subdirectory from the argument.
    func_dirname_result=`$ECHO "${1}" | $SED -e "$dirname"`
    if test "X$func_dirname_result" = "X${1}"; then
      func_dirname_result="${3}"
    else
      func_dirname_result="$func_dirname_result${2}"
    fi
    func_basename_result=`$ECHO "${1}" | $SED -e "$basename"`
} # func_dirname_and_basename may be replaced by extended shell implementation


# func_stripname prefix suffix name
# strip PREFIX and SUFFIX off of NAME.
# PREFIX and SUFFIX must not contain globbing or regex special
# characters, hashes, percent signs, but SUFFIX may contain a leading
# dot (in which case that matches only a dot).
# func_strip_suffix prefix name
func_stripname ()
{
    case ${2} in
      .*) func_stripname_result=`$ECHO "${3}" | $SED "s%^${1}%%; s%\\\\${2}\$%%"`;;
      *)  func_stripname_result=`$ECHO "${3}" | $SED "s%^${1}%%; s%${2}\$%%"`;;
    esac
} # func_stripname may be replaced by extended shell implementation


# These SED scripts presuppose an absolute path with a trailing slash.
pathcar='s,^/\([^/]*\).*$,\1,'
pathcdr='s,^/[^/]*,,'
removedotparts=':dotsl
		s@/\./@/@g
		t dotsl
		s,/\.$,/,'
collapseslashes='s@/\{1,\}@/@g'
finalslash='s,/*$,/,'

# func_normal_abspath PATH
# Remove doubled-up and trailing slashes, "." path components,
# and cancel out any ".." path components in PATH after making
# it an absolute path.
#             value returned in "$func_normal_abspath_result"
func_normal_abspath ()
{
  # Start from root dir and reassemble the path.
  func_normal_abspath_result=
  func_normal_abspath_tpath=$1
  func_normal_abspath_altnamespace=
  case $func_normal_abspath_tpath in
    "")
      # Empty path, that just means $cwd.
      func_stripname '' '/' "`pwd`"
      func_normal_abspath_result=$func_stripname_result
      return
    ;;
    # The next three entries are used to spot a run of precisely
    # two leading slashes without using negated character classes;
    # we take advantage of case's first-match behaviour.
    ///*)
      # Unusual form of absolute path, do nothing.
    ;;
    //*)
      # Not necessarily an ordinary path; POSIX reserves leading '//'
      # and for example Cygwin uses it to access remote file shares
      # over CIFS/SMB, so we conserve a leading double slash if found.
      func_normal_abspath_altnamespace=/
    ;;
    /*)
      # Absolute path, do nothing.
    ;;
    *)
      # Relative path, prepend $cwd.
      func_normal_abspath_tpath=`pwd`/$func_normal_abspath_tpath
    ;;
  esac
  # Cancel out all the simple stuff to save iterations.  We also want
  # the path to end with a slash for ease of parsing, so make sure
  # there is one (and only one) here.
  func_normal_abspath_tpath=`$ECHO "$func_normal_abspath_tpath" | $SED \
        -e "$removedotparts" -e "$collapseslashes" -e "$finalslash"`
  while :; do
    # Processed it all yet?
    if test "$func_normal_abspath_tpath" = / ; then
      # If we ascended to the root using ".." the result may be empty now.
      if test -z "$func_normal_abspath_result" ; then
        func_normal_abspath_result=/
      fi
      break
    fi
    func_normal_abspath_tcomponent=`$ECHO "$func_normal_abspath_tpath" | $SED \
        -e "$pathcar"`
    func_normal_abspath_tpath=`$ECHO "$func_normal_abspath_tpath" | $SED \
        -e "$pathcdr"`
    # Figure out what to do with it
    case $func_normal_abspath_tcomponent in
      "")
        # Trailing empty path component, ignore it.
      ;;
      ..)
        # Parent dir; strip last assembled component from result.
        func_dirname "$func_normal_abspath_result"
        func_normal_abspath_result=$func_dirname_result
      ;;
      *)
        # Actual path component, append it.
        func_normal_abspath_result=$func_normal_abspath_result/$func_normal_abspath_tcomponent
      ;;
    esac
  done
  # Restore leading double-slash if one was found on entry.
  func_normal_abspath_result=$func_normal_abspath_altnamespace$func_normal_abspath_result
}

# func_relative_path SRCDIR DSTDIR
# generates a relative path from SRCDIR to DSTDIR, with a trailing
# slash if non-empty, suitable for immediately appending a filename
# without needing to append a separator.
#             value returned in "$func_relative_path_result"
func_relative_path ()
{
  func_relative_path_result=
  func_normal_abspath "$1"
  func_relative_path_tlibdir=$func_normal_abspath_result
  func_normal_abspath "$2"
  func_relative_path_tbindir=$func_normal_abspath_result

  # Ascend the tree starting from libdir
  while :; do
    # check if we have found a prefix of bindir
    case $func_relative_path_tbindir in
      $func_relative_path_tlibdir)
        # found an exact match
        func_relative_path_tcancelled=
        break
        ;;
      $func_relative_path_tlibdir*)
        # found a matching prefix
        func_stripname "$func_relative_path_tlibdir" '' "$func_relative_path_tbindir"
        func_relative_path_tcancelled=$func_stripname_result
        if test -z "$func_relative_path_result"; then
          func_relative_path_result=.
        fi
        break
        ;;
      *)
        func_dirname $func_relative_path_tlibdir
        func_relative_path_tlibdir=${func_dirname_result}
        if test "x$func_relative_path_tlibdir" = x ; then
          # Have to descend all the way to the root!
          func_relative_path_result=../$func_relative_path_result
          func_relative_path_tcancelled=$func_relative_path_tbindir
          break
        fi
        func_relative_path_result=../$func_relative_path_result
        ;;
    esac
  done

  # Now calculate path; take care to avoid doubling-up slashes.
  func_stripname '' '/' "$func_relative_path_result"
  func_relative_path_result=$func_stripname_result
  func_stripname '/' '/' "$func_relative_path_tcancelled"
  if test "x$func_stripname_result" != x ; then
    func_relative_path_result=${func_relative_path_result}/${func_stripname_result}
  fi

  # Normalisation. If bindir is libdir, return empty string,
  # else relative path ending with a slash; either way, target
  # file name can be directly appended.
  if test ! -z "$func_relative_path_result"; then
    func_stripname './' '' "$func_relative_path_result/"
    func_relative_path_result=$func_stripname_result
  fi
}

# The name of this program:
func_dirname_and_basename "$progpath"
progname=$func_basename_result

# Make sure we have an absolute path for reexecution:
case $progpath in
  [\\/]*|[A-Za-z]:\\*) ;;
  *[\\/]*)
     progdir=$func_dirname_result
     progdir=`cd "$progdir" && pwd`
     progpath="$progdir/$progname"
     ;;
  *)
     save_IFS="$IFS"
     IFS=:
     for progdir in $PATH; do
       IFS="$save_IFS"
       test -x "$progdir/$progname" && break
     done
     IFS="$save_IFS"
     test -n "$progdir" || progdir=`pwd`
     progpath="$progdir/$progname"
     ;;
esac

# Sed substitution that helps us do robust quoting.  It backslashifies
# metacharacters that are still active within double-quoted strings.
Xsed="${SED}"' -e 1s/^X//'
sed_quote_subst='s/\([`"$\\]\)/\\\1/g'

# Same as above, but do not quote variable references.
double_quote_subst='s/\(["`\\]\)/\\\1/g'

# Sed substitution that turns a string into a regex matching for the
# string literally.
sed_make_literal_regex='s,[].[^$\\*\/],\\&,g'

# Sed substitution that converts a w32 file name or path
# which contains forward slashes, into one that contains
# (escaped) backslashes.  A very naive implementation.
lt_sed_naive_backslashify='s|\\\\*|\\|g;s|/|\\|g;s|\\|\\\\|g'

# Re-`\' parameter expansions in output of double_quote_subst that were
# `\'-ed in input to the same.  If an odd number of `\' preceded a '$'
# in input to double_quote_subst, that '$' was protected from expansion.
# Since each input `\' is now two `\'s, look for any number of runs of
# four `\'s followed by two `\'s and then a '$'.  `\' that '$'.
bs='\\'
bs2='\\\\'
bs4='\\\\\\\\'
dollar='\$'
sed_double_backslash="\
  s/$bs4/&\\
/g
  s/^$bs2$dollar/$bs&/
  s/\\([^$bs]\\)$bs2$dollar/\\1$bs2$bs$dollar/g
  s/\n//g"

# Standard options:
opt_dry_run=false
opt_help=false
opt_quiet=false
opt_verbose=false
opt_warning=:

# func_echo arg...
# Echo program name prefixed message, along with the current mode
# name if it has been set yet.
func_echo ()
{
    $ECHO "$progname: ${opt_mode+$opt_mode: }$*"
}

# func_verbose arg...
# Echo program name prefixed message in verbose mode only.
func_verbose ()
{
    $opt_verbose && func_echo ${1+"$@"}

    # A bug in bash halts the script if the last line of a function
    # fails when set -e is in force, so we need another command to
    # work around that:
    :
}

# func_echo_all arg...
# Invoke $ECHO with all args, space-separated.
func_echo_all ()
{
    $ECHO "$*"
}

# func_error arg...
# Echo program name prefixed message to standard error.
func_error ()
{
    $ECHO "$progname: ${opt_mode+$opt_mode: }"${1+"$@"} 1>&2
}

# func_warning arg...
# Echo program name prefixed warning message to standard error.
func_warning ()
{
    $opt_warning && $ECHO "$progname: ${opt_mode+$opt_mode: }warning: "${1+"$@"} 1>&2

    # bash bug again:
    :
}

# func_fatal_error arg...
# Echo program name prefixed message to standard error, and exit.
func_fatal_error ()
{
    func_error ${1+"$@"}
    exit $EXIT_FAILURE
}

# func_fatal_help arg...
# Echo program name prefixed message to standard error, followed by
# a help hint, and exit.
func_fatal_help ()
{
    func_error ${1+"$@"}
    func_fatal_error "$help"
}
help="Try \`$progname --help' for more information."  ## default


# func_grep expression filename
# Check whether EXPRESSION matches any line of FILENAME, without output.
func_grep ()
{
    $GREP "$1" "$2" >/dev/null 2>&1
}


# func_mkdir_p directory-path
# Make sure the entire path to DIRECTORY-PATH is available.
func_mkdir_p ()
{
    my_directory_path="$1"
    my_dir_list=

    if test -n "$my_directory_path" && test "$opt_dry_run" != ":"; then

      # Protect directory names starting with `-'
      case $my_directory_path in
        -*) my_directory_path="./$my_directory_path" ;;
      esac

      # While some portion of DIR does not yet exist...
      while test ! -d "$my_directory_path"; do
        # ...make a list in topmost first order.  Use a colon delimited
	# list incase some portion of path contains whitespace.
        my_dir_list="$my_directory_path:$my_dir_list"

        # If the last portion added has no slash in it, the list is done
        case $my_directory_path in */*) ;; *) break ;; esac

        # ...otherwise throw away the child directory and loop
        my_directory_path=`$ECHO "$my_directory_path" | $SED -e "$dirname"`
      done
      my_dir_list=`$ECHO "$my_dir_list" | $SED 's,:*$,,'`

      save_mkdir_p_IFS="$IFS"; IFS=':'
      for my_dir in $my_dir_list; do
	IFS="$save_mkdir_p_IFS"
        # mkdir can fail with a `File exist' error if two processes
        # try to create one of the directories concurrently.  Don't
        # stop in that case!
        $MKDIR "$my_dir" 2>/dev/null || :
      done
      IFS="$save_mkdir_p_IFS"

      # Bail out if we (or some other process) failed to create a directory.
      test -d "$my_directory_path" || \
        func_fatal_error "Failed to create \`$1'"
    fi
}


# func_mktempdir [string]
# Make a temporary directory that won't clash with other running
# libtool processes, and avoids race conditions if possible.  If
# given, STRING is the basename for that directory.
func_mktempdir ()
{
    my_template="${TMPDIR-/tmp}/${1-$progname}"

    if test "$opt_dry_run" = ":"; then
      # Return a directory name, but don't create it in dry-run mode
      my_tmpdir="${my_template}-$$"
    else

      # If mktemp works, use that first and foremost
      my_tmpdir=`mktemp -d "${my_template}-XXXXXXXX" 2>/dev/null`

      if test ! -d "$my_tmpdir"; then
        # Failing that, at least try and use $RANDOM to avoid a race
        my_tmpdir="${my_template}-${RANDOM-0}$$"

        save_mktempdir_umask=`umask`
        umask 0077
        $MKDIR "$my_tmpdir"
        umask $save_mktempdir_umask
      fi

      # If we're not in dry-run mode, bomb out on failure
      test -d "$my_tmpdir" || \
        func_fatal_error "cannot create temporary directory \`$my_tmpdir'"
    fi

    $ECHO "$my_tmpdir"
}


# func_quote_for_eval arg
# Aesthetically quote ARG to be evaled later.
# This function returns two values: FUNC_QUOTE_FOR_EVAL_RESULT
# is double-quoted, suitable for a subsequent eval, whereas
# FUNC_QUOTE_FOR_EVAL_UNQUOTED_RESULT has merely all characters
# which are still active within double quotes backslashified.
func_quote_for_eval ()
{
    case $1 in
      *[\\\`\"\$]*)
	func_quote_for_eval_unquoted_result=`$ECHO "$1" | $SED "$sed_quote_subst"` ;;
      *)
        func_quote_for_eval_unquoted_result="$1" ;;
    esac

    case $func_quote_for_eval_unquoted_result in
      # Double-quote args containing shell metacharacters to delay
      # word splitting, command substitution and and variable
      # expansion for a subsequent eval.
      # Many Bourne shells cannot handle close brackets correctly
      # in scan sets, so we specify it separately.
      *[\[\~\#\^\&\*\(\)\{\}\|\;\<\>\?\'\ \	]*|*]*|"")
        func_quote_for_eval_result="\"$func_quote_for_eval_unquoted_result\""
        ;;
      *)
        func_quote_for_eval_result="$func_quote_for_eval_unquoted_result"
    esac
}


# func_quote_for_expand arg
# Aesthetically quote ARG to be evaled later; same as above,
# but do not quote variable references.
func_quote_for_expand ()
{
    case $1 in
      *[\\\`\"]*)
	my_arg=`$ECHO "$1" | $SED \
	    -e "$double_quote_subst" -e "$sed_double_backslash"` ;;
      *)
        my_arg="$1" ;;
    esac

    case $my_arg in
      # Double-quote args containing shell metacharacters to delay
      # word splitting and command substitution for a subsequent eval.
      # Many Bourne shells cannot handle close brackets correctly
      # in scan sets, so we specify it separately.
      *[\[\~\#\^\&\*\(\)\{\}\|\;\<\>\?\'\ \	]*|*]*|"")
        my_arg="\"$my_arg\""
        ;;
    esac

    func_quote_for_expand_result="$my_arg"
}


# func_show_eval cmd [fail_exp]
# Unless opt_silent is true, then output CMD.  Then, if opt_dryrun is
# not true, evaluate CMD.  If the evaluation of CMD fails, and FAIL_EXP
# is given, then evaluate it.
func_show_eval ()
{
    my_cmd="$1"
    my_fail_exp="${2-:}"

    ${opt_silent-false} || {
      func_quote_for_expand "$my_cmd"
      eval "func_echo $func_quote_for_expand_result"
    }

    if ${opt_dry_run-false}; then :; else
      eval "$my_cmd"
      my_status=$?
      if test "$my_status" -eq 0; then :; else
	eval "(exit $my_status); $my_fail_exp"
      fi
    fi
}


# func_show_eval_locale cmd [fail_exp]
# Unless opt_silent is true, then output CMD.  Then, if opt_dryrun is
# not true, evaluate CMD.  If the evaluation of CMD fails, and FAIL_EXP
# is given, then evaluate it.  Use the saved locale for evaluation.
func_show_eval_locale ()
{
    my_cmd="$1"
    my_fail_exp="${2-:}"

    ${opt_silent-false} || {
      func_quote_for_expand "$my_cmd"
      eval "func_echo $func_quote_for_expand_result"
    }

    if ${opt_dry_run-false}; then :; else
      eval "$lt_user_locale
	    $my_cmd"
      my_status=$?
      eval "$lt_safe_locale"
      if test "$my_status" -eq 0; then :; else
	eval "(exit $my_status); $my_fail_exp"
      fi
    fi
}

# func_tr_sh
# Turn $1 into a string suitable for a shell variable name.
# Result is stored in $func_tr_sh_result.  All characters
# not in the set a-zA-Z0-9_ are replaced with '_'. Further,
# if $1 begins with a digit, a '_' is prepended as well.
func_tr_sh ()
{
  case $1 in
  [0-9]* | *[!a-zA-Z0-9_]*)
    func_tr_sh_result=`$ECHO "$1" | $SED 's/^\([0-9]\)/_\1/; s/[^a-zA-Z0-9_]/_/g'`
    ;;
  * )
    func_tr_sh_result=$1
    ;;
  esac
}


# func_version
# Echo version message to standard output and exit.
func_version ()
{
    $opt_debug

    $SED -n '/(C)/!b go
	:more
	/\./!{
	  N
	  s/\n# / /
	  b more
	}
	:go
	/^# '$PROGRAM' (GNU /,/# warranty; / {
        s/^# //
	s/^# *$//
        s/\((C)\)[ 0-9,-]*\( [1-9][0-9]*\)/\1\2/
        p
     }' < "$progpath"
     exit $?
}

# func_usage
# Echo short help message to standard output and exit.
func_usage ()
{
    $opt_debug

    $SED -n '/^# Usage:/,/^#  *.*--help/ {
        s/^# //
	s/^# *$//
	s/\$progname/'$progname'/
	p
    }' < "$progpath"
    echo
    $ECHO "run \`$progname --help | more' for full usage"
    exit $?
}

# func_help [NOEXIT]
# Echo long help message to standard output and exit,
# unless 'noexit' is passed as argument.
func_help ()
{
    $opt_debug

    $SED -n '/^# Usage:/,/# Report bugs to/ {
	:print
        s/^# //
	s/^# *$//
	s*\$progname*'$progname'*
	s*\$host*'"$host"'*
	s*\$SHELL*'"$SHELL"'*
	s*\$LTCC*'"$LTCC"'*
	s*\$LTCFLAGS*'"$LTCFLAGS"'*
	s*\$LD*'"$LD"'*
	s/\$with_gnu_ld/'"$with_gnu_ld"'/
	s/\$automake_version/'"`(automake --version) 2>/dev/null |$SED 1q`"'/
	s/\$autoconf_version/'"`(autoconf --version) 2>/dev/null |$SED 1q`"'/
	p
	d
     }
     /^# .* home page:/b print
     /^# General help using/b print
     ' < "$progpath"
    ret=$?
    if test -z "$1"; then
      exit $ret
    fi
}

# func_missing_arg argname
# Echo program name prefixed message to standard error and set global
# exit_cmd.
func_missing_arg ()
{
    $opt_debug

    func_error "missing argument for $1."
    exit_cmd=exit
}


# func_split_short_opt shortopt
# Set func_split_short_opt_name and func_split_short_opt_arg shell
# variables after splitting SHORTOPT after the 2nd character.
func_split_short_opt ()
{
    my_sed_short_opt='1s/^\(..\).*$/\1/;q'
    my_sed_short_rest='1s/^..\(.*\)$/\1/;q'

    func_split_short_opt_name=`$ECHO "$1" | $SED "$my_sed_short_opt"`
    func_split_short_opt_arg=`$ECHO "$1" | $SED "$my_sed_short_rest"`
} # func_split_short_opt may be replaced by extended shell implementation


# func_split_long_opt longopt
# Set func_split_long_opt_name and func_split_long_opt_arg shell
# variables after splitting LONGOPT at the `=' sign.
func_split_long_opt ()
{
    my_sed_long_opt='1s/^\(--[^=]*\)=.*/\1/;q'
    my_sed_long_arg='1s/^--[^=]*=//'

    func_split_long_opt_name=`$ECHO "$1" | $SED "$my_sed_long_opt"`
    func_split_long_opt_arg=`$ECHO "$1" | $SED "$my_sed_long_arg"`
} # func_split_long_opt may be replaced by extended shell implementation

exit_cmd=:





magic="%%%MAGIC variable%%%"
magic_exe="%%%MAGIC EXE variable%%%"

# Global variables.
nonopt=
preserve_args=
lo2o="s/\\.lo\$/.${objext}/"
o2lo="s/\\.${objext}\$/.lo/"
extracted_archives=
extracted_serial=0

# If this variable is set in any of the actions, the command in it
# will be execed at the end.  This prevents here-documents from being
# left over by shells.
exec_cmd=

# func_append var value
# Append VALUE to the end of shell variable VAR.
func_append ()
{
    eval "${1}=\$${1}\${2}"
} # func_append may be replaced by extended shell implementation

# func_append_quoted var value
# Quote VALUE and append to the end of shell variable VAR, separated
# by a space.
func_append_quoted ()
{
    func_quote_for_eval "${2}"
    eval "${1}=\$${1}\\ \$func_quote_for_eval_result"
} # func_append_quoted may be replaced by extended shell implementation


# func_arith arithmetic-term...
func_arith ()
{
    func_arith_result=`expr "${@}"`
} # func_arith may be replaced by extended shell implementation


# func_len string
# STRING may not start with a hyphen.
func_len ()
{
    func_len_result=`expr "${1}" : ".*" 2>/dev/null || echo $max_cmd_len`
} # func_len may be replaced by extended shell implementation


# func_lo2o object
func_lo2o ()
{
    func_lo2o_result=`$ECHO "${1}" | $SED "$lo2o"`
} # func_lo2o may be replaced by extended shell implementation


# func_xform libobj-or-source
func_xform ()
{
    func_xform_result=`$ECHO "${1}" | $SED 's/\.[^.]*$/.lo/'`
} # func_xform may be replaced by extended shell implementation


# func_fatal_configuration arg...
# Echo program name prefixed message to standard error, followed by
# a configuration failure hint, and exit.
func_fatal_configuration ()
{
    func_error ${1+"$@"}
    func_error "See the $PACKAGE documentation for more information."
    func_fatal_error "Fatal configuration error."
}


# func_config
# Display the configuration for all the tags in this script.
func_config ()
{
    re_begincf='^# ### BEGIN LIBTOOL'
    re_endcf='^# ### END LIBTOOL'

    # Default configuration.
    $SED "1,/$re_begincf CONFIG/d;/$re_endcf CONFIG/,\$d" < "$progpath"

    # Now print the configurations for the tags.
    for tagname in $taglist; do
      $SED -n "/$re_begincf TAG CONFIG: $tagname\$/,/$re_endcf TAG CONFIG: $tagname\$/p" < "$progpath"
    done

    exit $?
}

# func_features
# Display the features supported by this script.
func_features ()
{
    echo "host: $host"
    if test "$build_libtool_libs" = yes; then
      echo "enable shared libraries"
    else
      echo "disable shared libraries"
    fi
    if test "$build_old_libs" = yes; then
      echo "enable static libraries"
    else
      echo "disable static libraries"
    fi

    exit $?
}

# func_enable_tag tagname
# Verify that TAGNAME is valid, and either flag an error and exit, or
# enable the TAGNAME tag.  We also add TAGNAME to the global $taglist
# variable here.
func_enable_tag ()
{
  # Global variable:
  tagname="$1"

  re_begincf="^# ### BEGIN LIBTOOL TAG CONFIG: $tagname\$"
  re_endcf="^# ### END LIBTOOL TAG CONFIG: $tagname\$"
  sed_extractcf="/$re_begincf/,/$re_endcf/p"

  # Validate tagname.
  case $tagname in
    *[!-_A-Za-z0-9,/]*)
      func_fatal_error "invalid tag name: $tagname"
      ;;
  esac

  # Don't test for the "default" C tag, as we know it's
  # there but not specially marked.
  case $tagname in
    CC) ;;
    *)
      if $GREP "$re_begincf" "$progpath" >/dev/null 2>&1; then
	taglist="$taglist $tagname"

	# Evaluate the configuration.  Be careful to quote the path
	# and the sed script, to avoid splitting on whitespace, but
	# also don't use non-portable quotes within backquotes within
	# quotes we have to do it in 2 steps:
	extractedcf=`$SED -n -e "$sed_extractcf" < "$progpath"`
	eval "$extractedcf"
      else
	func_error "ignoring unknown tag $tagname"
      fi
      ;;
  esac
}

# func_check_version_match
# Ensure that we are using m4 macros, and libtool script from the same
# release of libtool.
func_check_version_match ()
{
  if test "$package_revision" != "$macro_revision"; then
    if test "$VERSION" != "$macro_version"; then
      if test -z "$macro_version"; then
        cat >&2 <<_LT_EOF
$progname: Version mismatch error.  This is $PACKAGE $VERSION, but the
$progname: definition of this LT_INIT comes from an older release.
$progname: You should recreate aclocal.m4 with macros from $PACKAGE $VERSION
$progname: and run autoconf again.
_LT_EOF
      else
        cat >&2 <<_LT_EOF
$progname: Version mismatch error.  This is $PACKAGE $VERSION, but the
$progname: definition of this LT_INIT comes from $PACKAGE $macro_version.
$progname: You should recreate aclocal.m4 with macros from $PACKAGE $VERSION
$progname: and run autoconf again.
_LT_EOF
      fi
    else
      cat >&2 <<_LT_EOF
$progname: Version mismatch error.  This is $PACKAGE $VERSION, revision $package_revision,
$progname: but the definition of this LT_INIT comes from revision $macro_revision.
$progname: You should recreate aclocal.m4 with macros from revision $package_revision
$progname: of $PACKAGE $VERSION and run autoconf again.
_LT_EOF
    fi

    exit $EXIT_MISMATCH
  fi
}


# Shorthand for --mode=foo, only valid as the first argument
case $1 in
clean|clea|cle|cl)
  shift; set dummy --mode clean ${1+"$@"}; shift
  ;;
compile|compil|compi|comp|com|co|c)
  shift; set dummy --mode compile ${1+"$@"}; shift
  ;;
execute|execut|execu|exec|exe|ex|e)
  shift; set dummy --mode execute ${1+"$@"}; shift
  ;;
finish|finis|fini|fin|fi|f)
  shift; set dummy --mode finish ${1+"$@"}; shift
  ;;
install|instal|insta|inst|ins|in|i)
  shift; set dummy --mode install ${1+"$@"}; shift
  ;;
link|lin|li|l)
  shift; set dummy --mode link ${1+"$@"}; shift
  ;;
uninstall|uninstal|uninsta|uninst|unins|unin|uni|un|u)
  shift; set dummy --mode uninstall ${1+"$@"}; shift
  ;;
esac



# Option defaults:
opt_debug=:
opt_dry_run=false
opt_config=false
opt_preserve_dup_deps=false
opt_features=false
opt_finish=false
opt_help=false
opt_help_all=false
opt_silent=:
opt_verbose=:
opt_silent=false
opt_verbose=false


# Parse options once, thoroughly.  This comes as soon as possible in the
# script to make things like `--version' happen as quickly as we can.
{
  # this just eases exit handling
  while test $# -gt 0; do
    opt="$1"
    shift
    case $opt in
      --debug|-x)	opt_debug='set -x'
			func_echo "enabling shell trace mode"
			$opt_debug
			;;
      --dry-run|--dryrun|-n)
			opt_dry_run=:
			;;
      --config)
			opt_config=:
func_config
			;;
      --dlopen|-dlopen)
			optarg="$1"
			opt_dlopen="${opt_dlopen+$opt_dlopen
}$optarg"
			shift
			;;
      --preserve-dup-deps)
			opt_preserve_dup_deps=:
			;;
      --features)
			opt_features=:
func_features
			;;
      --finish)
			opt_finish=:
set dummy --mode finish ${1+"$@"}; shift
			;;
      --help)
			opt_help=:
			;;
      --help-all)
			opt_help_all=:
opt_help=': help-all'
			;;
      --mode)
			test $# = 0 && func_missing_arg $opt && break
			optarg="$1"
			opt_mode="$optarg"
case $optarg in
  # Valid mode arguments:
  clean|compile|execute|finish|install|link|relink|uninstall) ;;

  # Catch anything else as an error
  *) func_error "invalid argument for $opt"
     exit_cmd=exit
     break
     ;;
esac
			shift
			;;
      --no-silent|--no-quiet)
			opt_silent=false
func_append preserve_args " $opt"
			;;
      --no-verbose)
			opt_verbose=false
func_append preserve_args " $opt"
			;;
      --silent|--quiet)
			opt_silent=:
func_append preserve_args " $opt"
        opt_verbose=false
			;;
      --verbose|-v)
			opt_verbose=:
func_append preserve_args " $opt"
opt_silent=false
			;;
      --tag)
			test $# = 0 && func_missing_arg $opt && break
			optarg="$1"
			opt_tag="$optarg"
func_append preserve_args " $opt $optarg"
func_enable_tag "$optarg"
			shift
			;;

      -\?|-h)		func_usage				;;
      --help)		func_help				;;
      --version)	func_version				;;

      # Separate optargs to long options:
      --*=*)
			func_split_long_opt "$opt"
			set dummy "$func_split_long_opt_name" "$func_split_long_opt_arg" ${1+"$@"}
			shift
			;;

      # Separate non-argument short options:
      -\?*|-h*|-n*|-v*)
			func_split_short_opt "$opt"
			set dummy "$func_split_short_opt_name" "-$func_split_short_opt_arg" ${1+"$@"}
			shift
			;;

      --)		break					;;
      -*)		func_fatal_help "unrecognized option \`$opt'" ;;
      *)		set dummy "$opt" ${1+"$@"};	shift; break  ;;
    esac
  done

  # Validate options:

  # save first non-option argument
  if test "$#" -gt 0; then
    nonopt="$opt"
    shift
  fi

  # preserve --debug
  test "$opt_debug" = : || func_append preserve_args " --debug"

  case $host in
    *cygwin* | *mingw* | *pw32* | *cegcc*)
      # don't eliminate duplications in $postdeps and $predeps
      opt_duplicate_compiler_generated_deps=:
      ;;
    *)
      opt_duplicate_compiler_generated_deps=$opt_preserve_dup_deps
      ;;
  esac

  $opt_help || {
    # Sanity checks first:
    func_check_version_match

    if test "$build_libtool_libs" != yes && test "$build_old_libs" != yes; then
      func_fatal_configuration "not configured to build any kind of library"
    fi

    # Darwin sucks
    eval std_shrext=\"$shrext_cmds\"

    # Only execute mode is allowed to have -dlopen flags.
    if test -n "$opt_dlopen" && test "$opt_mode" != execute; then
      func_error "unrecognized option \`-dlopen'"
      $ECHO "$help" 1>&2
      exit $EXIT_FAILURE
    fi

    # Change the help message to a mode-specific one.
    generic_help="$help"
    help="Try \`$progname --help --mode=$opt_mode' for more information."
  }


  # Bail if the options were screwed
  $exit_cmd $EXIT_FAILURE
}




## ----------- ##
##    Main.    ##
## ----------- ##

# func_lalib_p file
# True iff FILE is a libtool `.la' library or `.lo' object file.
# This function is only a basic sanity check; it will hardly flush out
# determined imposters.
func_lalib_p ()
{
    test -f "$1" &&
      $SED -e 4q "$1" 2>/dev/null \
        | $GREP "^# Generated by .*$PACKAGE" > /dev/null 2>&1
}

# func_lalib_unsafe_p file
# True iff FILE is a libtool `.la' library or `.lo' object file.
# This function implements the same check as func_lalib_p without
# resorting to external programs.  To this end, it redirects stdin and
# closes it afterwards, without saving the original file descriptor.
# As a safety measure, use it only where a negative result would be
# fatal anyway.  Works if `file' does not exist.
func_lalib_unsafe_p ()
{
    lalib_p=no
    if test -f "$1" && test -r "$1" && exec 5<&0 <"$1"; then
	for lalib_p_l in 1 2 3 4
	do
	    read lalib_p_line
	    case "$lalib_p_line" in
		\#\ Generated\ by\ *$PACKAGE* ) lalib_p=yes; break;;
	    esac
	done
	exec 0<&5 5<&-
    fi
    test "$lalib_p" = yes
}

# func_ltwrapper_script_p file
# True iff FILE is a libtool wrapper script
# This function is only a basic sanity check; it will hardly flush out
# determined imposters.
func_ltwrapper_script_p ()
{
    func_lalib_p "$1"
}

# func_ltwrapper_executable_p file
# True iff FILE is a libtool wrapper executable
# This function is only a basic sanity check; it will hardly flush out
# determined imposters.
func_ltwrapper_executable_p ()
{
    func_ltwrapper_exec_suffix=
    case $1 in
    *.exe) ;;
    *) func_ltwrapper_exec_suffix=.exe ;;
    esac
    $GREP "$magic_exe" "$1$func_ltwrapper_exec_suffix" >/dev/null 2>&1
}

# func_ltwrapper_scriptname file
# Assumes file is an ltwrapper_executable
# uses $file to determine the appropriate filename for a
# temporary ltwrapper_script.
func_ltwrapper_scriptname ()
{
    func_dirname_and_basename "$1" "" "."
    func_stripname '' '.exe' "$func_basename_result"
    func_ltwrapper_scriptname_result="$func_dirname_result/$objdir/${func_stripname_result}_ltshwrapper"
}

# func_ltwrapper_p file
# True iff FILE is a libtool wrapper script or wrapper executable
# This function is only a basic sanity check; it will hardly flush out
# determined imposters.
func_ltwrapper_p ()
{
    func_ltwrapper_script_p "$1" || func_ltwrapper_executable_p "$1"
}


# func_execute_cmds commands fail_cmd
# Execute tilde-delimited COMMANDS.
# If FAIL_CMD is given, eval that upon failure.
# FAIL_CMD may read-access the current command in variable CMD!
func_execute_cmds ()
{
    $opt_debug
    save_ifs=$IFS; IFS='~'
    for cmd in $1; do
      IFS=$save_ifs
      eval cmd=\"$cmd\"
      func_show_eval "$cmd" "${2-:}"
    done
    IFS=$save_ifs
}


# func_source file
# Source FILE, adding directory component if necessary.
# Note that it is not necessary on cygwin/mingw to append a dot to
# FILE even if both FILE and FILE.exe exist: automatic-append-.exe
# behavior happens only for exec(3), not for open(2)!  Also, sourcing
# `FILE.' does not work on cygwin managed mounts.
func_source ()
{
    $opt_debug
    case $1 in
    */* | *\\*)	. "$1" ;;
    *)		. "./$1" ;;
    esac
}


# func_resolve_sysroot PATH
# Replace a leading = in PATH with a sysroot.  Store the result into
# func_resolve_sysroot_result
func_resolve_sysroot ()
{
  func_resolve_sysroot_result=$1
  case $func_resolve_sysroot_result in
  =*)
    func_stripname '=' '' "$func_resolve_sysroot_result"
    func_resolve_sysroot_result=$lt_sysroot$func_stripname_result
    ;;
  esac
}

# func_replace_sysroot PATH
# If PATH begins with the sysroot, replace it with = and
# store the result into func_replace_sysroot_result.
func_replace_sysroot ()
{
  case "$lt_sysroot:$1" in
  ?*:"$lt_sysroot"*)
    func_stripname "$lt_sysroot" '' "$1"
    func_replace_sysroot_result="=$func_stripname_result"
    ;;
  *)
    # Including no sysroot.
    func_replace_sysroot_result=$1
    ;;
  esac
}

# func_infer_tag arg
# Infer tagged configuration to use if any are available and
# if one wasn't chosen via the "--tag" command line option.
# Only attempt this if the compiler in the base compile
# command doesn't match the default compiler.
# arg is usually of the form 'gcc ...'
func_infer_tag ()
{
    $opt_debug
    if test -n "$available_tags" && test -z "$tagname"; then
      CC_quoted=
      for arg in $CC; do
	func_append_quoted CC_quoted "$arg"
      done
      CC_expanded=`func_echo_all $CC`
      CC_quoted_expanded=`func_echo_all $CC_quoted`
      case $@ in
      # Blanks in the command may have been stripped by the calling shell,
      # but not from the CC environment variable when configure was run.
      " $CC "* | "$CC "* | " $CC_expanded "* | "$CC_expanded "* | \
      " $CC_quoted"* | "$CC_quoted "* | " $CC_quoted_expanded "* | "$CC_quoted_expanded "*) ;;
      # Blanks at the start of $base_compile will cause this to fail
      # if we don't check for them as well.
      *)
	for z in $available_tags; do
	  if $GREP "^# ### BEGIN LIBTOOL TAG CONFIG: $z$" < "$progpath" > /dev/null; then
	    # Evaluate the configuration.
	    eval "`${SED} -n -e '/^# ### BEGIN LIBTOOL TAG CONFIG: '$z'$/,/^# ### END LIBTOOL TAG CONFIG: '$z'$/p' < $progpath`"
	    CC_quoted=
	    for arg in $CC; do
	      # Double-quote args containing other shell metacharacters.
	      func_append_quoted CC_quoted "$arg"
	    done
	    CC_expanded=`func_echo_all $CC`
	    CC_quoted_expanded=`func_echo_all $CC_quoted`
	    case "$@ " in
	    " $CC "* | "$CC "* | " $CC_expanded "* | "$CC_expanded "* | \
	    " $CC_quoted"* | "$CC_quoted "* | " $CC_quoted_expanded "* | "$CC_quoted_expanded "*)
	      # The compiler in the base compile command matches
	      # the one in the tagged configuration.
	      # Assume this is the tagged configuration we want.
	      tagname=$z
	      break
	      ;;
	    esac
	  fi
	done
	# If $tagname still isn't set, then no tagged configuration
	# was found and let the user know that the "--tag" command
	# line option must be used.
	if test -z "$tagname"; then
	  func_echo "unable to infer tagged configuration"
	  func_fatal_error "specify a tag with \`--tag'"
#	else
#	  func_verbose "using $tagname tagged configuration"
	fi
	;;
      esac
    fi
}



# func_write_libtool_object output_name pic_name nonpic_name
# Create a libtool object file (analogous to a ".la" file),
# but don't create it if we're doing a dry run.
func_write_libtool_object ()
{
    write_libobj=${1}
    if test "$build_libtool_libs" = yes; then
      write_lobj=\'${2}\'
    else
      write_lobj=none
    fi

    if test "$build_old_libs" = yes; then
      write_oldobj=\'${3}\'
    else
      write_oldobj=none
    fi

    $opt_dry_run || {
      cat >${write_libobj}T <<EOF
# $write_libobj - a libtool object file
# Generated by $PROGRAM (GNU $PACKAGE$TIMESTAMP) $VERSION
#
# Please DO NOT delete this file!
# It is necessary for linking the library.

# Name of the PIC object.
pic_object=$write_lobj

# Name of the non-PIC object
non_pic_object=$write_oldobj

EOF
      $MV "${write_libobj}T" "${write_libobj}"
    }
}


##################################################
# FILE NAME AND PATH CONVERSION HELPER FUNCTIONS #
##################################################

# func_convert_core_file_wine_to_w32 ARG
# Helper function used by file name conversion functions when $build is *nix,
# and $host is mingw, cygwin, or some other w32 environment. Relies on a
# correctly configured wine environment available, with the winepath program
# in $build's $PATH.
#
# ARG is the $build file name to be converted to w32 format.
# Result is available in $func_convert_core_file_wine_to_w32_result, and will
# be empty on error (or when ARG is empty)
func_convert_core_file_wine_to_w32 ()
{
  $opt_debug
  func_convert_core_file_wine_to_w32_result="$1"
  if test -n "$1"; then
    # Unfortunately, winepath does not exit with a non-zero error code, so we
    # are forced to check the contents of stdout. On the other hand, if the
    # command is not found, the shell will set an exit code of 127 and print
    # *an error message* to stdout. So we must check for both error code of
    # zero AND non-empty stdout, which explains the odd construction:
    func_convert_core_file_wine_to_w32_tmp=`winepath -w "$1" 2>/dev/null`
    if test "$?" -eq 0 && test -n "${func_convert_core_file_wine_to_w32_tmp}"; then
      func_convert_core_file_wine_to_w32_result=`$ECHO "$func_convert_core_file_wine_to_w32_tmp" |
        $SED -e "$lt_sed_naive_backslashify"`
    else
      func_convert_core_file_wine_to_w32_result=
    fi
  fi
}
# end: func_convert_core_file_wine_to_w32


# func_convert_core_path_wine_to_w32 ARG
# Helper function used by path conversion functions when $build is *nix, and
# $host is mingw, cygwin, or some other w32 environment. Relies on a correctly
# configured wine environment available, with the winepath program in $build's
# $PATH. Assumes ARG has no leading or trailing path separator characters.
#
# ARG is path to be converted from $build format to win32.
# Result is available in $func_convert_core_path_wine_to_w32_result.
# Unconvertible file (directory) names in ARG are skipped; if no directory names
# are convertible, then the result may be empty.
func_convert_core_path_wine_to_w32 ()
{
  $opt_debug
  # unfortunately, winepath doesn't convert paths, only file names
  func_convert_core_path_wine_to_w32_result=""
  if test -n "$1"; then
    oldIFS=$IFS
    IFS=:
    for func_convert_core_path_wine_to_w32_f in $1; do
      IFS=$oldIFS
      func_convert_core_file_wine_to_w32 "$func_convert_core_path_wine_to_w32_f"
      if test -n "$func_convert_core_file_wine_to_w32_result" ; then
        if test -z "$func_convert_core_path_wine_to_w32_result"; then
          func_convert_core_path_wine_to_w32_result="$func_convert_core_file_wine_to_w32_result"
        else
          func_append func_convert_core_path_wine_to_w32_result ";$func_convert_core_file_wine_to_w32_result"
        fi
      fi
    done
    IFS=$oldIFS
  fi
}
# end: func_convert_core_path_wine_to_w32


# func_cygpath ARGS...
# Wrapper around calling the cygpath program via LT_CYGPATH. This is used when
# when (1) $build is *nix and Cygwin is hosted via a wine environment; or (2)
# $build is MSYS and $host is Cygwin, or (3) $build is Cygwin. In case (1) or
# (2), returns the Cygwin file name or path in func_cygpath_result (input
# file name or path is assumed to be in w32 format, as previously converted
# from $build's *nix or MSYS format). In case (3), returns the w32 file name
# or path in func_cygpath_result (input file name or path is assumed to be in
# Cygwin format). Returns an empty string on error.
#
# ARGS are passed to cygpath, with the last one being the file name or path to
# be converted.
#
# Specify the absolute *nix (or w32) name to cygpath in the LT_CYGPATH
# environment variable; do not put it in $PATH.
func_cygpath ()
{
  $opt_debug
  if test -n "$LT_CYGPATH" && test -f "$LT_CYGPATH"; then
    func_cygpath_result=`$LT_CYGPATH "$@" 2>/dev/null`
    if test "$?" -ne 0; then
      # on failure, ensure result is empty
      func_cygpath_result=
    fi
  else
    func_cygpath_result=
    func_error "LT_CYGPATH is empty or specifies non-existent file: \`$LT_CYGPATH'"
  fi
}
#end: func_cygpath


# func_convert_core_msys_to_w32 ARG
# Convert file name or path ARG from MSYS format to w32 format.  Return
# result in func_convert_core_msys_to_w32_result.
func_convert_core_msys_to_w32 ()
{
  $opt_debug
  # awkward: cmd appends spaces to result
  func_convert_core_msys_to_w32_result=`( cmd //c echo "$1" ) 2>/dev/null |
    $SED -e 's/[ ]*$//' -e "$lt_sed_naive_backslashify"`
}
#end: func_convert_core_msys_to_w32


# func_convert_file_check ARG1 ARG2
# Verify that ARG1 (a file name in $build format) was converted to $host
# format in ARG2. Otherwise, emit an error message, but continue (resetting
# func_to_host_file_result to ARG1).
func_convert_file_check ()
{
  $opt_debug
  if test -z "$2" && test -n "$1" ; then
    func_error "Could not determine host file name corresponding to"
    func_error "  \`$1'"
    func_error "Continuing, but uninstalled executables may not work."
    # Fallback:
    func_to_host_file_result="$1"
  fi
}
# end func_convert_file_check


# func_convert_path_check FROM_PATHSEP TO_PATHSEP FROM_PATH TO_PATH
# Verify that FROM_PATH (a path in $build format) was converted to $host
# format in TO_PATH. Otherwise, emit an error message, but continue, resetting
# func_to_host_file_result to a simplistic fallback value (see below).
func_convert_path_check ()
{
  $opt_debug
  if test -z "$4" && test -n "$3"; then
    func_error "Could not determine the host path corresponding to"
    func_error "  \`$3'"
    func_error "Continuing, but uninstalled executables may not work."
    # Fallback.  This is a deliberately simplistic "conversion" and
    # should not be "improved".  See libtool.info.
    if test "x$1" != "x$2"; then
      lt_replace_pathsep_chars="s|$1|$2|g"
      func_to_host_path_result=`echo "$3" |
        $SED -e "$lt_replace_pathsep_chars"`
    else
      func_to_host_path_result="$3"
    fi
  fi
}
# end func_convert_path_check


# func_convert_path_front_back_pathsep FRONTPAT BACKPAT REPL ORIG
# Modifies func_to_host_path_result by prepending REPL if ORIG matches FRONTPAT
# and appending REPL if ORIG matches BACKPAT.
func_convert_path_front_back_pathsep ()
{
  $opt_debug
  case $4 in
  $1 ) func_to_host_path_result="$3$func_to_host_path_result"
    ;;
  esac
  case $4 in
  $2 ) func_append func_to_host_path_result "$3"
    ;;
  esac
}
# end func_convert_path_front_back_pathsep


##################################################
# $build to $host FILE NAME CONVERSION FUNCTIONS #
##################################################
# invoked via `$to_host_file_cmd ARG'
#
# In each case, ARG is the path to be converted from $build to $host format.
# Result will be available in $func_to_host_file_result.


# func_to_host_file ARG
# Converts the file name ARG from $build format to $host format. Return result
# in func_to_host_file_result.
func_to_host_file ()
{
  $opt_debug
  $to_host_file_cmd "$1"
}
# end func_to_host_file


# func_to_tool_file ARG LAZY
# converts the file name ARG from $build format to toolchain format. Return
# result in func_to_tool_file_result.  If the conversion in use is listed
# in (the comma separated) LAZY, no conversion takes place.
func_to_tool_file ()
{
  $opt_debug
  case ,$2, in
    *,"$to_tool_file_cmd",*)
      func_to_tool_file_result=$1
      ;;
    *)
      $to_tool_file_cmd "$1"
      func_to_tool_file_result=$func_to_host_file_result
      ;;
  esac
}
# end func_to_tool_file


# func_convert_file_noop ARG
# Copy ARG to func_to_host_file_result.
func_convert_file_noop ()
{
  func_to_host_file_result="$1"
}
# end func_convert_file_noop


# func_convert_file_msys_to_w32 ARG
# Convert file name ARG from (mingw) MSYS to (mingw) w32 format; automatic
# conversion to w32 is not available inside the cwrapper.  Returns result in
# func_to_host_file_result.
func_convert_file_msys_to_w32 ()
{
  $opt_debug
  func_to_host_file_result="$1"
  if test -n "$1"; then
    func_convert_core_msys_to_w32 "$1"
    func_to_host_file_result="$func_convert_core_msys_to_w32_result"
  fi
  func_convert_file_check "$1" "$func_to_host_file_result"
}
# end func_convert_file_msys_to_w32


# func_convert_file_cygwin_to_w32 ARG
# Convert file name ARG from Cygwin to w32 format.  Returns result in
# func_to_host_file_result.
func_convert_file_cygwin_to_w32 ()
{
  $opt_debug
  func_to_host_file_result="$1"
  if test -n "$1"; then
    # because $build is cygwin, we call "the" cygpath in $PATH; no need to use
    # LT_CYGPATH in this case.
    func_to_host_file_result=`cygpath -m "$1"`
  fi
  func_convert_file_check "$1" "$func_to_host_file_result"
}
# end func_convert_file_cygwin_to_w32


# func_convert_file_nix_to_w32 ARG
# Convert file name ARG from *nix to w32 format.  Requires a wine environment
# and a working winepath. Returns result in func_to_host_file_result.
func_convert_file_nix_to_w32 ()
{
  $opt_debug
  func_to_host_file_result="$1"
  if test -n "$1"; then
    func_convert_core_file_wine_to_w32 "$1"
    func_to_host_file_result="$func_convert_core_file_wine_to_w32_result"
  fi
  func_convert_file_check "$1" "$func_to_host_file_result"
}
# end func_convert_file_nix_to_w32


# func_convert_file_msys_to_cygwin ARG
# Convert file name ARG from MSYS to Cygwin format.  Requires LT_CYGPATH set.
# Returns result in func_to_host_file_result.
func_convert_file_msys_to_cygwin ()
{
  $opt_debug
  func_to_host_file_result="$1"
  if test -n "$1"; then
    func_convert_core_msys_to_w32 "$1"
    func_cygpath -u "$func_convert_core_msys_to_w32_result"
    func_to_host_file_result="$func_cygpath_result"
  fi
  func_convert_file_check "$1" "$func_to_host_file_result"
}
# end func_convert_file_msys_to_cygwin


# func_convert_file_nix_to_cygwin ARG
# Convert file name ARG from *nix to Cygwin format.  Requires Cygwin installed
# in a wine environment, working winepath, and LT_CYGPATH set.  Returns result
# in func_to_host_file_result.
func_convert_file_nix_to_cygwin ()
{
  $opt_debug
  func_to_host_file_result="$1"
  if test -n "$1"; then
    # convert from *nix to w32, then use cygpath to convert from w32 to cygwin.
    func_convert_core_file_wine_to_w32 "$1"
    func_cygpath -u "$func_convert_core_file_wine_to_w32_result"
    func_to_host_file_result="$func_cygpath_result"
  fi
  func_convert_file_check "$1" "$func_to_host_file_result"
}
# end func_convert_file_nix_to_cygwin


#############################################
# $build to $host PATH CONVERSION FUNCTIONS #
#############################################
# invoked via `$to_host_path_cmd ARG'
#
# In each case, ARG is the path to be converted from $build to $host format.
# The result will be available in $func_to_host_path_result.
#
# Path separators are also converted from $build format to $host format.  If
# ARG begins or ends with a path separator character, it is preserved (but
# converted to $host format) on output.
#
# All path conversion functions are named using the following convention:
#   file name conversion function    : func_convert_file_X_to_Y ()
#   path conversion function         : func_convert_path_X_to_Y ()
# where, for any given $build/$host combination the 'X_to_Y' value is the
# same.  If conversion functions are added for new $build/$host combinations,
# the two new functions must follow this pattern, or func_init_to_host_path_cmd
# will break.


# func_init_to_host_path_cmd
# Ensures that function "pointer" variable $to_host_path_cmd is set to the
# appropriate value, based on the value of $to_host_file_cmd.
to_host_path_cmd=
func_init_to_host_path_cmd ()
{
  $opt_debug
  if test -z "$to_host_path_cmd"; then
    func_stripname 'func_convert_file_' '' "$to_host_file_cmd"
    to_host_path_cmd="func_convert_path_${func_stripname_result}"
  fi
}


# func_to_host_path ARG
# Converts the path ARG from $build format to $host format. Return result
# in func_to_host_path_result.
func_to_host_path ()
{
  $opt_debug
  func_init_to_host_path_cmd
  $to_host_path_cmd "$1"
}
# end func_to_host_path


# func_convert_path_noop ARG
# Copy ARG to func_to_host_path_result.
func_convert_path_noop ()
{
  func_to_host_path_result="$1"
}
# end func_convert_path_noop


# func_convert_path_msys_to_w32 ARG
# Convert path ARG from (mingw) MSYS to (mingw) w32 format; automatic
# conversion to w32 is not available inside the cwrapper.  Returns result in
# func_to_host_path_result.
func_convert_path_msys_to_w32 ()
{
  $opt_debug
  func_to_host_path_result="$1"
  if test -n "$1"; then
    # Remove leading and trailing path separator characters from ARG.  MSYS
    # behavior is inconsistent here; cygpath turns them into '.;' and ';.';
    # and winepath ignores them completely.
    func_stripname : : "$1"
    func_to_host_path_tmp1=$func_stripname_result
    func_convert_core_msys_to_w32 "$func_to_host_path_tmp1"
    func_to_host_path_result="$func_convert_core_msys_to_w32_result"
    func_convert_path_check : ";" \
      "$func_to_host_path_tmp1" "$func_to_host_path_result"
    func_convert_path_front_back_pathsep ":*" "*:" ";" "$1"
  fi
}
# end func_convert_path_msys_to_w32


# func_convert_path_cygwin_to_w32 ARG
# Convert path ARG from Cygwin to w32 format.  Returns result in
# func_to_host_file_result.
func_convert_path_cygwin_to_w32 ()
{
  $opt_debug
  func_to_host_path_result="$1"
  if test -n "$1"; then
    # See func_convert_path_msys_to_w32:
    func_stripname : : "$1"
    func_to_host_path_tmp1=$func_stripname_result
    func_to_host_path_result=`cygpath -m -p "$func_to_host_path_tmp1"`
    func_convert_path_check : ";" \
      "$func_to_host_path_tmp1" "$func_to_host_path_result"
    func_convert_path_front_back_pathsep ":*" "*:" ";" "$1"
  fi
}
# end func_convert_path_cygwin_to_w32


# func_convert_path_nix_to_w32 ARG
# Convert path ARG from *nix to w32 format.  Requires a wine environment and
# a working winepath.  Returns result in func_to_host_file_result.
func_convert_path_nix_to_w32 ()
{
  $opt_debug
  func_to_host_path_result="$1"
  if test -n "$1"; then
    # See func_convert_path_msys_to_w32:
    func_stripname : : "$1"
    func_to_host_path_tmp1=$func_stripname_result
    func_convert_core_path_wine_to_w32 "$func_to_host_path_tmp1"
    func_to_host_path_result="$func_convert_core_path_wine_to_w32_result"
    func_convert_path_check : ";" \
      "$func_to_host_path_tmp1" "$func_to_host_path_result"
    func_convert_path_front_back_pathsep ":*" "*:" ";" "$1"
  fi
}
# end func_convert_path_nix_to_w32


# func_convert_path_msys_to_cygwin ARG
# Convert path ARG from MSYS to Cygwin format.  Requires LT_CYGPATH set.
# Returns result in func_to_host_file_result.
func_convert_path_msys_to_cygwin ()
{
  $opt_debug
  func_to_host_path_result="$1"
  if test -n "$1"; then
    # See func_convert_path_msys_to_w32:
    func_stripname : : "$1"
    func_to_host_path_tmp1=$func_stripname_result
    func_convert_core_msys_to_w32 "$func_to_host_path_tmp1"
    func_cygpath -u -p "$func_convert_core_msys_to_w32_result"
    func_to_host_path_result="$func_cygpath_result"
    func_convert_path_check : : \
      "$func_to_host_path_tmp1" "$func_to_host_path_result"
    func_convert_path_front_back_pathsep ":*" "*:" : "$1"
  fi
}
# end func_convert_path_msys_to_cygwin


# func_convert_path_nix_to_cygwin ARG
# Convert path ARG from *nix to Cygwin format.  Requires Cygwin installed in a
# a wine environment, working winepath, and LT_CYGPATH set.  Returns result in
# func_to_host_file_result.
func_convert_path_nix_to_cygwin ()
{
  $opt_debug
  func_to_host_path_result="$1"
  if test -n "$1"; then
    # Remove leading and trailing path separator characters from
    # ARG. msys behavior is inconsistent here, cygpath turns them
    # into '.;' and ';.', and winepath ignores them completely.
    func_stripname : : "$1"
    func_to_host_path_tmp1=$func_stripname_result
    func_convert_core_path_wine_to_w32 "$func_to_host_path_tmp1"
    func_cygpath -u -p "$func_convert_core_path_wine_to_w32_result"
    func_to_host_path_result="$func_cygpath_result"
    func_convert_path_check : : \
      "$func_to_host_path_tmp1" "$func_to_host_path_result"
    func_convert_path_front_back_pathsep ":*" "*:" : "$1"
  fi
}
# end func_convert_path_nix_to_cygwin


# func_mode_compile arg...
func_mode_compile ()
{
    $opt_debug
    # Get the compilation command and the source file.
    base_compile=
    srcfile="$nonopt"  #  always keep a non-empty value in "srcfile"
    suppress_opt=yes
    suppress_output=
    arg_mode=normal
    libobj=
    later=
    pie_flag=

    for arg
    do
      case $arg_mode in
      arg  )
	# do not "continue".  Instead, add this to base_compile
	lastarg="$arg"
	arg_mode=normal
	;;

      target )
	libobj="$arg"
	arg_mode=normal
	continue
	;;

      normal )
	# Accept any command-line options.
	case $arg in
	-o)
	  test -n "$libobj" && \
	    func_fatal_error "you cannot specify \`-o' more than once"
	  arg_mode=target
	  continue
	  ;;

	-pie | -fpie | -fPIE)
          func_append pie_flag " $arg"
	  continue
	  ;;

	-shared | -static | -prefer-pic | -prefer-non-pic)
	  func_append later " $arg"
	  continue
	  ;;

	-no-suppress)
	  suppress_opt=no
	  continue
	  ;;

	-Xcompiler)
	  arg_mode=arg  #  the next one goes into the "base_compile" arg list
	  continue      #  The current "srcfile" will either be retained or
	  ;;            #  replaced later.  I would guess that would be a bug.

	-Wc,*)
	  func_stripname '-Wc,' '' "$arg"
	  args=$func_stripname_result
	  lastarg=
	  save_ifs="$IFS"; IFS=','
	  for arg in $args; do
	    IFS="$save_ifs"
	    func_append_quoted lastarg "$arg"
	  done
	  IFS="$save_ifs"
	  func_stripname ' ' '' "$lastarg"
	  lastarg=$func_stripname_result

	  # Add the arguments to base_compile.
	  func_append base_compile " $lastarg"
	  continue
	  ;;

	*)
	  # Accept the current argument as the source file.
	  # The previous "srcfile" becomes the current argument.
	  #
	  lastarg="$srcfile"
	  srcfile="$arg"
	  ;;
	esac  #  case $arg
	;;
      esac    #  case $arg_mode

      # Aesthetically quote the previous argument.
      func_append_quoted base_compile "$lastarg"
    done # for arg

    case $arg_mode in
    arg)
      func_fatal_error "you must specify an argument for -Xcompile"
      ;;
    target)
      func_fatal_error "you must specify a target with \`-o'"
      ;;
    *)
      # Get the name of the library object.
      test -z "$libobj" && {
	func_basename "$srcfile"
	libobj="$func_basename_result"
      }
      ;;
    esac

    # Recognize several different file suffixes.
    # If the user specifies -o file.o, it is replaced with file.lo
    case $libobj in
    *.[cCFSifmso] | \
    *.ada | *.adb | *.ads | *.asm | \
    *.c++ | *.cc | *.ii | *.class | *.cpp | *.cxx | \
    *.[fF][09]? | *.for | *.java | *.obj | *.sx | *.cu | *.cup)
      func_xform "$libobj"
      libobj=$func_xform_result
      ;;
    esac

    case $libobj in
    *.lo) func_lo2o "$libobj"; obj=$func_lo2o_result ;;
    *)
      func_fatal_error "cannot determine name of library object from \`$libobj'"
      ;;
    esac

    func_infer_tag $base_compile

    for arg in $later; do
      case $arg in
      -shared)
	test "$build_libtool_libs" != yes && \
	  func_fatal_configuration "can not build a shared library"
	build_old_libs=no
	continue
	;;

      -static)
	build_libtool_libs=no
	build_old_libs=yes
	continue
	;;

      -prefer-pic)
	pic_mode=yes
	continue
	;;

      -prefer-non-pic)
	pic_mode=no
	continue
	;;
      esac
    done

    func_quote_for_eval "$libobj"
    test "X$libobj" != "X$func_quote_for_eval_result" \
      && $ECHO "X$libobj" | $GREP '[]~#^*{};<>?"'"'"'	 &()|`$[]' \
      && func_warning "libobj name \`$libobj' may not contain shell special characters."
    func_dirname_and_basename "$obj" "/" ""
    objname="$func_basename_result"
    xdir="$func_dirname_result"
    lobj=${xdir}$objdir/$objname

    test -z "$base_compile" && \
      func_fatal_help "you must specify a compilation command"

    # Delete any leftover library objects.
    if test "$build_old_libs" = yes; then
      removelist="$obj $lobj $libobj ${libobj}T"
    else
      removelist="$lobj $libobj ${libobj}T"
    fi

    # On Cygwin there's no "real" PIC flag so we must build both object types
    case $host_os in
    cygwin* | mingw* | pw32* | os2* | cegcc*)
      pic_mode=default
      ;;
    esac
    if test "$pic_mode" = no && test "$deplibs_check_method" != pass_all; then
      # non-PIC code in shared libraries is not supported
      pic_mode=default
    fi

    # Calculate the filename of the output object if compiler does
    # not support -o with -c
    if test "$compiler_c_o" = no; then
      output_obj=`$ECHO "$srcfile" | $SED 's%^.*/%%; s%\.[^.]*$%%'`.${objext}
      lockfile="$output_obj.lock"
    else
      output_obj=
      need_locks=no
      lockfile=
    fi

    # Lock this critical section if it is needed
    # We use this script file to make the link, it avoids creating a new file
    if test "$need_locks" = yes; then
      until $opt_dry_run || ln "$progpath" "$lockfile" 2>/dev/null; do
	func_echo "Waiting for $lockfile to be removed"
	sleep 2
      done
    elif test "$need_locks" = warn; then
      if test -f "$lockfile"; then
	$ECHO "\
*** ERROR, $lockfile exists and contains:
`cat $lockfile 2>/dev/null`

This indicates that another process is trying to use the same
temporary object file, and libtool could not work around it because
your compiler does not support \`-c' and \`-o' together.  If you
repeat this compilation, it may succeed, by chance, but you had better
avoid parallel builds (make -j) in this platform, or get a better
compiler."

	$opt_dry_run || $RM $removelist
	exit $EXIT_FAILURE
      fi
      func_append removelist " $output_obj"
      $ECHO "$srcfile" > "$lockfile"
    fi

    $opt_dry_run || $RM $removelist
    func_append removelist " $lockfile"
    trap '$opt_dry_run || $RM $removelist; exit $EXIT_FAILURE' 1 2 15

    func_to_tool_file "$srcfile" func_convert_file_msys_to_w32
    srcfile=$func_to_tool_file_result
    func_quote_for_eval "$srcfile"
    qsrcfile=$func_quote_for_eval_result

    # Only build a PIC object if we are building libtool libraries.
    if test "$build_libtool_libs" = yes; then
      # Without this assignment, base_compile gets emptied.
      fbsd_hideous_sh_bug=$base_compile

      if test "$pic_mode" != no; then
	command="$base_compile $qsrcfile $pic_flag"
      else
	# Don't build PIC code
	command="$base_compile $qsrcfile"
      fi

      func_mkdir_p "$xdir$objdir"

      if test -z "$output_obj"; then
	# Place PIC objects in $objdir
	func_append command " -o $lobj"
      fi

      func_show_eval_locale "$command"	\
          'test -n "$output_obj" && $RM $removelist; exit $EXIT_FAILURE'

      if test "$need_locks" = warn &&
	 test "X`cat $lockfile 2>/dev/null`" != "X$srcfile"; then
	$ECHO "\
*** ERROR, $lockfile contains:
`cat $lockfile 2>/dev/null`

but it should contain:
$srcfile

This indicates that another process is trying to use the same
temporary object file, and libtool could not work around it because
your compiler does not support \`-c' and \`-o' together.  If you
repeat this compilation, it may succeed, by chance, but you had better
avoid parallel builds (make -j) in this platform, or get a better
compiler."

	$opt_dry_run || $RM $removelist
	exit $EXIT_FAILURE
      fi

      # Just move the object if needed, then go on to compile the next one
      if test -n "$output_obj" && test "X$output_obj" != "X$lobj"; then
	func_show_eval '$MV "$output_obj" "$lobj"' \
	  'error=$?; $opt_dry_run || $RM $removelist; exit $error'
      fi

      # Allow error messages only from the first compilation.
      if test "$suppress_opt" = yes; then
	suppress_output=' >/dev/null 2>&1'
      fi
    fi

    # Only build a position-dependent object if we build old libraries.
    if test "$build_old_libs" = yes; then
      if test "$pic_mode" != yes; then
	# Don't build PIC code
	command="$base_compile $qsrcfile$pie_flag"
      else
	command="$base_compile $qsrcfile $pic_flag"
      fi
      if test "$compiler_c_o" = yes; then
	func_append command " -o $obj"
      fi

      # Suppress compiler output if we already did a PIC compilation.
      func_append command "$suppress_output"
      func_show_eval_locale "$command" \
        '$opt_dry_run || $RM $removelist; exit $EXIT_FAILURE'

      if test "$need_locks" = warn &&
	 test "X`cat $lockfile 2>/dev/null`" != "X$srcfile"; then
	$ECHO "\
*** ERROR, $lockfile contains:
`cat $lockfile 2>/dev/null`

but it should contain:
$srcfile

This indicates that another process is trying to use the same
temporary object file, and libtool could not work around it because
your compiler does not support \`-c' and \`-o' together.  If you
repeat this compilation, it may succeed, by chance, but you had better
avoid parallel builds (make -j) in this platform, or get a better
compiler."

	$opt_dry_run || $RM $removelist
	exit $EXIT_FAILURE
      fi

      # Just move the object if needed
      if test -n "$output_obj" && test "X$output_obj" != "X$obj"; then
	func_show_eval '$MV "$output_obj" "$obj"' \
	  'error=$?; $opt_dry_run || $RM $removelist; exit $error'
      fi
    fi

    $opt_dry_run || {
      func_write_libtool_object "$libobj" "$objdir/$objname" "$objname"

      # Unlock the critical section if it was locked
      if test "$need_locks" != no; then
	removelist=$lockfile
        $RM "$lockfile"
      fi
    }

    exit $EXIT_SUCCESS
}

$opt_help || {
  test "$opt_mode" = compile && func_mode_compile ${1+"$@"}
}

func_mode_help ()
{
    # We need to display help for each of the modes.
    case $opt_mode in
      "")
        # Generic help is extracted from the usage comments
        # at the start of this file.
        func_help
        ;;

      clean)
        $ECHO \
"Usage: $progname [OPTION]... --mode=clean RM [RM-OPTION]... FILE...

Remove files from the build directory.

RM is the name of the program to use to delete files associated with each FILE
(typically \`/bin/rm').  RM-OPTIONS are options (such as \`-f') to be passed
to RM.

If FILE is a libtool library, object or program, all the files associated
with it are deleted. Otherwise, only FILE itself is deleted using RM."
        ;;

      compile)
      $ECHO \
"Usage: $progname [OPTION]... --mode=compile COMPILE-COMMAND... SOURCEFILE

Compile a source file into a libtool library object.

This mode accepts the following additional options:

  -o OUTPUT-FILE    set the output file name to OUTPUT-FILE
  -no-suppress      do not suppress compiler output for multiple passes
  -prefer-pic       try to build PIC objects only
  -prefer-non-pic   try to build non-PIC objects only
  -shared           do not build a \`.o' file suitable for static linking
  -static           only build a \`.o' file suitable for static linking
  -Wc,FLAG          pass FLAG directly to the compiler

COMPILE-COMMAND is a command to be used in creating a \`standard' object file
from the given SOURCEFILE.

The output file name is determined by removing the directory component from
SOURCEFILE, then substituting the C source code suffix \`.c' with the
library object suffix, \`.lo'."
        ;;

      execute)
        $ECHO \
"Usage: $progname [OPTION]... --mode=execute COMMAND [ARGS]...

Automatically set library path, then run a program.

This mode accepts the following additional options:

  -dlopen FILE      add the directory containing FILE to the library path

This mode sets the library path environment variable according to \`-dlopen'
flags.

If any of the ARGS are libtool executable wrappers, then they are translated
into their corresponding uninstalled binary, and any of their required library
directories are added to the library path.

Then, COMMAND is executed, with ARGS as arguments."
        ;;

      finish)
        $ECHO \
"Usage: $progname [OPTION]... --mode=finish [LIBDIR]...

Complete the installation of libtool libraries.

Each LIBDIR is a directory that contains libtool libraries.

The commands that this mode executes may require superuser privileges.  Use
the \`--dry-run' option if you just want to see what would be executed."
        ;;

      install)
        $ECHO \
"Usage: $progname [OPTION]... --mode=install INSTALL-COMMAND...

Install executables or libraries.

INSTALL-COMMAND is the installation command.  The first component should be
either the \`install' or \`cp' program.

The following components of INSTALL-COMMAND are treated specially:

  -inst-prefix-dir PREFIX-DIR  Use PREFIX-DIR as a staging area for installation

The rest of the components are interpreted as arguments to that command (only
BSD-compatible install options are recognized)."
        ;;

      link)
        $ECHO \
"Usage: $progname [OPTION]... --mode=link LINK-COMMAND...

Link object files or libraries together to form another library, or to
create an executable program.

LINK-COMMAND is a command using the C compiler that you would use to create
a program from several object files.

The following components of LINK-COMMAND are treated specially:

  -all-static       do not do any dynamic linking at all
  -avoid-version    do not add a version suffix if possible
  -bindir BINDIR    specify path to binaries directory (for systems where
                    libraries must be found in the PATH setting at runtime)
  -dlopen FILE      \`-dlpreopen' FILE if it cannot be dlopened at runtime
  -dlpreopen FILE   link in FILE and add its symbols to lt_preloaded_symbols
  -export-dynamic   allow symbols from OUTPUT-FILE to be resolved with dlsym(3)
  -export-symbols SYMFILE
                    try to export only the symbols listed in SYMFILE
  -export-symbols-regex REGEX
                    try to export only the symbols matching REGEX
  -LLIBDIR          search LIBDIR for required installed libraries
  -lNAME            OUTPUT-FILE requires the installed library libNAME
  -module           build a library that can dlopened
  -no-fast-install  disable the fast-install mode
  -no-install       link a not-installable executable
  -no-undefined     declare that a library does not refer to external symbols
  -o OUTPUT-FILE    create OUTPUT-FILE from the specified objects
  -objectlist FILE  Use a list of object files found in FILE to specify objects
  -precious-files-regex REGEX
                    don't remove output files matching REGEX
  -release RELEASE  specify package release information
  -rpath LIBDIR     the created library will eventually be installed in LIBDIR
  -R[ ]LIBDIR       add LIBDIR to the runtime path of programs and libraries
  -shared           only do dynamic linking of libtool libraries
  -shrext SUFFIX    override the standard shared library file extension
  -static           do not do any dynamic linking of uninstalled libtool libraries
  -static-libtool-libs
                    do not do any dynamic linking of libtool libraries
  -version-info CURRENT[:REVISION[:AGE]]
                    specify library version info [each variable defaults to 0]
  -weak LIBNAME     declare that the target provides the LIBNAME interface
  -Wc,FLAG
  -Xcompiler FLAG   pass linker-specific FLAG directly to the compiler
  -Wl,FLAG
  -Xlinker FLAG     pass linker-specific FLAG directly to the linker
  -XCClinker FLAG   pass link-specific FLAG to the compiler driver (CC)

All other options (arguments beginning with \`-') are ignored.

Every other argument is treated as a filename.  Files ending in \`.la' are
treated as uninstalled libtool libraries, other files are standard or library
object files.

If the OUTPUT-FILE ends in \`.la', then a libtool library is created,
only library objects (\`.lo' files) may be specified, and \`-rpath' is
required, except when creating a convenience library.

If OUTPUT-FILE ends in \`.a' or \`.lib', then a standard library is created
using \`ar' and \`ranlib', or on Windows using \`lib'.

If OUTPUT-FILE ends in \`.lo' or \`.${objext}', then a reloadable object file
is created, otherwise an executable program is created."
        ;;

      uninstall)
        $ECHO \
"Usage: $progname [OPTION]... --mode=uninstall RM [RM-OPTION]... FILE...

Remove libraries from an installation directory.

RM is the name of the program to use to delete files associated with each FILE
(typically \`/bin/rm').  RM-OPTIONS are options (such as \`-f') to be passed
to RM.

If FILE is a libtool library, all the files associated with it are deleted.
Otherwise, only FILE itself is deleted using RM."
        ;;

      *)
        func_fatal_help "invalid operation mode \`$opt_mode'"
        ;;
    esac

    echo
    $ECHO "Try \`$progname --help' for more information about other modes."
}

# Now that we've collected a possible --mode arg, show help if necessary
if $opt_help; then
  if test "$opt_help" = :; then
    func_mode_help
  else
    {
      func_help noexit
      for opt_mode in compile link execute install finish uninstall clean; do
	func_mode_help
      done
    } | sed -n '1p; 2,$s/^Usage:/  or: /p'
    {
      func_help noexit
      for opt_mode in compile link execute install finish uninstall clean; do
	echo
	func_mode_help
      done
    } |
    sed '1d
      /^When reporting/,/^Report/{
	H
	d
      }
      $x
      /information about other modes/d
      /more detailed .*MODE/d
      s/^Usage:.*--mode=\([^ ]*\) .*/Description of \1 mode:/'
  fi
  exit $?
fi


# func_mode_execute arg...
func_mode_execute ()
{
    $opt_debug
    # The first argument is the command name.
    cmd="$nonopt"
    test -z "$cmd" && \
      func_fatal_help "you must specify a COMMAND"

    # Handle -dlopen flags immediately.
    for file in $opt_dlopen; do
      test -f "$file" \
	|| func_fatal_help "\`$file' is not a file"

      dir=
      case $file in
      *.la)
	func_resolve_sysroot "$file"
	file=$func_resolve_sysroot_result

	# Check to see that this really is a libtool archive.
	func_lalib_unsafe_p "$file" \
	  || func_fatal_help "\`$lib' is not a valid libtool archive"

	# Read the libtool library.
	dlname=
	library_names=
	func_source "$file"

	# Skip this library if it cannot be dlopened.
	if test -z "$dlname"; then
	  # Warn if it was a shared library.
	  test -n "$library_names" && \
	    func_warning "\`$file' was not linked with \`-export-dynamic'"
	  continue
	fi

	func_dirname "$file" "" "."
	dir="$func_dirname_result"

	if test -f "$dir/$objdir/$dlname"; then
	  func_append dir "/$objdir"
	else
	  if test ! -f "$dir/$dlname"; then
	    func_fatal_error "cannot find \`$dlname' in \`$dir' or \`$dir/$objdir'"
	  fi
	fi
	;;

      *.lo)
	# Just add the directory containing the .lo file.
	func_dirname "$file" "" "."
	dir="$func_dirname_result"
	;;

      *)
	func_warning "\`-dlopen' is ignored for non-libtool libraries and objects"
	continue
	;;
      esac

      # Get the absolute pathname.
      absdir=`cd "$dir" && pwd`
      test -n "$absdir" && dir="$absdir"

      # Now add the directory to shlibpath_var.
      if eval "test -z \"\$$shlibpath_var\""; then
	eval "$shlibpath_var=\"\$dir\""
      else
	eval "$shlibpath_var=\"\$dir:\$$shlibpath_var\""
      fi
    done

    # This variable tells wrapper scripts just to set shlibpath_var
    # rather than running their programs.
    libtool_execute_magic="$magic"

    # Check if any of the arguments is a wrapper script.
    args=
    for file
    do
      case $file in
      -* | *.la | *.lo ) ;;
      *)
	# Do a test to see if this is really a libtool program.
	if func_ltwrapper_script_p "$file"; then
	  func_source "$file"
	  # Transform arg to wrapped name.
	  file="$progdir/$program"
	elif func_ltwrapper_executable_p "$file"; then
	  func_ltwrapper_scriptname "$file"
	  func_source "$func_ltwrapper_scriptname_result"
	  # Transform arg to wrapped name.
	  file="$progdir/$program"
	fi
	;;
      esac
      # Quote arguments (to preserve shell metacharacters).
      func_append_quoted args "$file"
    done

    if test "X$opt_dry_run" = Xfalse; then
      if test -n "$shlibpath_var"; then
	# Export the shlibpath_var.
	eval "export $shlibpath_var"
      fi

      # Restore saved environment variables
      for lt_var in LANG LANGUAGE LC_ALL LC_CTYPE LC_COLLATE LC_MESSAGES
      do
	eval "if test \"\${save_$lt_var+set}\" = set; then
                $lt_var=\$save_$lt_var; export $lt_var
	      else
		$lt_unset $lt_var
	      fi"
      done

      # Now prepare to actually exec the command.
      exec_cmd="\$cmd$args"
    else
      # Display what would be done.
      if test -n "$shlibpath_var"; then
	eval "\$ECHO \"\$shlibpath_var=\$$shlibpath_var\""
	echo "export $shlibpath_var"
      fi
      $ECHO "$cmd$args"
      exit $EXIT_SUCCESS
    fi
}

test "$opt_mode" = execute && func_mode_execute ${1+"$@"}


# func_mode_finish arg...
func_mode_finish ()
{
    $opt_debug
    libs=
    libdirs=
    admincmds=

    for opt in "$nonopt" ${1+"$@"}
    do
      if test -d "$opt"; then
	func_append libdirs " $opt"

      elif test -f "$opt"; then
	if func_lalib_unsafe_p "$opt"; then
	  func_append libs " $opt"
	else
	  func_warning "\`$opt' is not a valid libtool archive"
	fi

      else
	func_fatal_error "invalid argument \`$opt'"
      fi
    done

    if test -n "$libs"; then
      if test -n "$lt_sysroot"; then
        sysroot_regex=`$ECHO "$lt_sysroot" | $SED "$sed_make_literal_regex"`