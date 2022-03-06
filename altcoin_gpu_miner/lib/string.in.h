/* A GNU-like <string.h>.

   Copyright (C) 1995-1996, 2001-2011 Free Software Foundation, Inc.

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

#ifndef _@GUARD_PREFIX@_STRING_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_STRING_H@

#ifndef _@GUARD_PREFIX@_STRING_H
#define _@GUARD_PREFIX@_STRING_H

/* NetBSD 5.0 mis-defines NULL.  */
#include <stddef.h>

/* MirBSD defines mbslen as a macro.  */
#if @GNULIB_MBSLEN@ && defined __MirBSD__
# include <wchar.h>
#endif

/* The __attribute__ feature is available in gcc versions 2.5 and later.
   The attribute __pure__ was added in gcc 2.96.  */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
# define _GL_ATTRIBUTE_PURE __attribute__ ((__pure__))
#else
# define _GL_ATTRIBUTE_PURE /* empty */
#endif

/* NetBSD 5.0 declares strsignal in <unistd.h>, not in <string.h>.  */
/* But in any case avoid namespace pollution on glibc systems.  */
#if (@GNULIB_STRSIGNAL@ || defined GNULIB_POSIXCHECK) && defined __NetBSD__ \
    && ! defined __GLIBC__
# include <unistd.h>
#endif

/* The definitions of _GL_FUNCDECL_RPL etc. are copied here.  */

/* The definition of _GL_ARG_NONNULL is copied here.  */

/* The definition of _GL_WARN_ON_USE is copied here.  */


/* Return the first instance of C within N bytes of S, or NULL.  */
#if @GNULIB_MEMCHR@
# if @REPLACE_MEMCHR@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   define memchr rpl_memchr
#  endif
_GL_FUNCDECL_RPL (memchr, void *, (void const *__s, int __c, size_t __n)
                                  _GL_ATTRIBUTE_PURE
                                  _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (memchr, void *, (void const *__s, int __c, size_t __n));
# else
#  if ! @HAVE_MEMCHR@
_GL_FUNCDECL_SYS (memchr, void *, (void const *__s, int __c, size_t __n)
                                  _GL_ATTRIBUTE_PURE
                                  _GL_ARG_NONNULL ((1)));
#  endif
  /* On some systems, this function is defined as an overloaded function:
       extern "C" { const void * std::memchr (const void *, int, size_t); }
       extern "C++" { void * std::memchr (void *, int, size_t); }  */
_GL_CXXALIAS_SYS_CAST2 (memchr,
                        void *, (void const *__s, int __c, size_t __n),
                        void const *, (void const *__s, int __c, size_t __n));
# endif
# if ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 10) && !defined __UCLIBC__) \
     && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
_GL_CXXALIASWARN1 (memchr, void *, (void *__s, int __c, size_t __n));
_GL_CXXALIASWARN1 (memchr, void const *,
                   (void const *__s, int __c, size_t __n));
# else
_GL_CXXALIASWARN (memchr);
# endif
#elif defined GNULIB_POSIXCHECK
# undef memchr
/* Assume memchr is always declared.  */
_GL_WARN_ON_USE (memchr, "memchr has platform-specific bugs - "
                 "use gnulib module memchr for portability" );
#endif

/* Return the first occurrence of NEEDLE in HAYSTACK.  */
#if @GNULIB_MEMMEM@
# if @REPLACE_MEMMEM@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   define memmem rpl_memmem
#  endif
_GL_FUNCDECL_RPL (memmem, void *,
                  (void const *__haystack, size_t __haystack_len,
                   void const *__needle, size_t __needle_len)
                  _GL_ATTRIBUTE_PURE
                  _GL_ARG_NONNULL ((1, 3)));
_GL_CXXALIAS_RPL (memmem, void *,
                  (void const *__haystack, size_t __haystack_len,
                   void const *__needle, size_t __needle_len));
# else
#  if ! @HAVE_DECL_MEMMEM@
_GL_FUNCDECL_SYS (memmem, void *,
                  (void const *__haystack, size_t __haystack_len,
                   void const *__needle, size_t __needle_len)
                  _GL_ATTRIBUTE_PURE
                  _GL_ARG_NONNULL ((1, 3)));
#  endif
_GL_CXXALIAS_SYS (memmem, void *,
                  (void const *__haystack, size_t __haystack_len,
                   void const *__needle, size_t __needle_len));
# endif
_GL_CXXALIASWARN (memmem);
#elif defined GNULIB_POSIXCHECK
# undef memmem
# if HAVE_RAW_DECL_MEMMEM
_GL_WARN_ON_USE (memmem, "memmem is unportable and often quadratic - "
                 "use gnulib module memmem-simple for portability, "
                 "and module memmem for speed" );
# endif
#endif

/* Copy N bytes of SRC to DEST, return pointer to bytes after the
   last written byte.  */
#if @GNULIB_MEMPCPY@
# if ! @HAVE_MEMPCPY@
_GL_FUNCDECL_SYS (mempcpy, void *,
                  (void *restrict __dest, void const *restrict __src,
                   size_t __n)
                  _GL_ARG_NONNULL ((1, 2)));
# endif
_GL_CXXALIAS_SYS (mempcpy, void *,
                  (void *restrict __dest, void const *restrict __src,
                   size_t __n));
_GL_CXXALIASWARN (mempcpy);
#elif defined GNULIB_POSIXCHECK
# undef mempcpy
# if HAVE_RAW_DECL_MEMPCPY
_GL_WARN_ON_USE (mempcpy, "mempcpy is unportable - "
                 "use gnulib module mempcpy for portability");
# endif
#endif

/* Search backwards through a block for a byte (specified as an int).  */
#if @GNULIB_MEMRCHR@
# if ! @HAVE_DECL_MEMRCHR@
_GL_FUNCDECL_SYS (memrchr, void *, (void const *, int, size_t)
                                   _GL_ATTRIBUTE_PURE
                                   _GL_ARG_NONNULL ((1)));
# endif
  /* On some systems, this function is defined as an overloaded function:
       extern "C++" { const void * std::memrchr (const void *, int, size_t); }
       extern "C++" { void * std::memrchr (void *, int, size_t); }  */
_GL_CXXALIAS_SYS_CAST2 (memrchr,
                        void *, (void const *, int, size_t),
                        void const *, (void const *, int, size_t));
# if ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 10) && !defined __UCLIBC__) \
     && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
_GL_CXXALIASWARN1 (memrchr, void *, (void *, int, size_t));
_GL_CXXALIASWARN1 (memrchr, void const *, (void const *, int, size_t));
# else
_GL_CXXALIASWARN (memrchr);
# endif
#elif defined GNULIB_POSIXCHECK
# undef memrchr
# if HAVE_RAW_DECL_MEMRCHR
_GL_WARN_ON_USE (memrchr, "memrchr is unportable - "
                 "use gnulib module memrchr for portability");
# endif
#endif

/* Find the first occurrence of C in S.  More efficient than
   memchr(S,C,N), at the expense of undefined behavior if C does not
   occur within N bytes.  */
#if @GNULIB_RAWMEMCHR@
# if ! @HAVE_RAWMEMCHR@
_GL_FUNCDECL_SYS (rawmemchr, void *, (void const *__s, int __c_in)
                                     _GL_ATTRIBUTE_PURE
                                     _GL_ARG_NONNULL ((1)));
# endif
  /* On some systems, this function is defined as an overloaded function:
       extern "C++" { const void * std::rawmemchr (const void *, int); }
       extern "C++" { void * std::rawmemchr (void *, int); }  */
_GL_CXXALIAS_SYS_CAST2 (rawmemchr,
                        void *, (void const *__s, int __c_in),
                        void const *, (void const *__s, int __c_in));
# if ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 10) && !defined __UCLIBC__) \
     && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
_GL_CXXALIASWARN1 (rawmemchr, void *, (void *__s, int __c_in));
_GL_CXXALIASWARN1 (rawmemchr, void const *, (void const *__s, int __c_in));
# else
_GL_CXXALIASWARN (rawmemchr);
# endif
#elif defined GNULIB_POSIXCHECK
# undef rawmemchr
# if HAVE_RAW_DECL_RAWMEMCHR
_GL_WARN_ON_USE (rawmemchr, "rawmemchr is unportable - "
                 "use gnulib module rawmemchr for portability");
# endif
#endif

/* Copy SRC to DST, returning the address of the terminating '\0' in DST.  */
#if @GNULIB_STPCPY@
# if ! @HAVE_STPCPY@
_GL_FUNCDECL_SYS (stpcpy, char *,
                  (char *restrict __dst, char const *restrict __src)
                  _GL_ARG_NONNULL ((1, 2)));
# endif
_GL_CXXALIAS_SYS (stpcpy, char *,
                  (char *restrict __dst, char const *restrict __src));
_GL_CXXALIASWARN (stpcpy);
#elif defined GNULIB_POSIXCHECK
# undef stpcpy
# if HAVE_RAW_DECL_STPCPY
_GL_WARN_ON_USE (stpcpy, "stpcpy is unportable - "
                 "use gnulib module stpcpy for portability");
# endif
#endif

/* Copy no more than N bytes of SRC to DST, returning a pointer past the
   last non-NUL byte written into DST.  */
#if @GNULIB_STPNCPY@
# if @REPLACE_STPNCPY@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef stpncpy
#   define stpncpy rpl_stpncpy
#  endif
_GL_FUNCDECL_RPL (stpncpy, char *,
                  (char *restrict __dst, char const *restrict __src,
                   size_t __n)
                  _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (stpncpy, char *,
                  (char *restrict __dst, char const *restrict __src,
                   size_t __n));
# else
#  if ! @HAVE_STPNCPY@
_GL_FUNCDECL_SYS (stpncpy, char *,
                  (char *restrict __dst, char const *restrict __src,
                   size_t __n)
                  _GL_ARG_NONNULL ((1, 2)));
#  endif
_GL_CXXALIAS_SYS (stpncpy, char *,
                  (char *restrict __dst, char const *restrict __src,
                   size_t __n));
# endif
_GL_CXXALIASWARN (stpncpy);
#elif defined GNULIB_POSIXCHECK
# undef stpncpy
# if HAVE_RAW_DECL_STPNCPY
_GL_WARN_ON_USE (stpncpy, "stpncpy is unportable - "
                 "use gnulib module stpncpy for portability");
# endif
#endif

#if defined GNULIB_POSIXCHECK
/* strchr() does not work with multibyte strings if the locale encoding is
   GB18030 and the character to be searched is a digit.  */
# undef strchr
/* Assume strchr is always declared.  */
_GL_WARN_ON_USE (strchr, "strchr cannot work correctly on character strings "
                 "in some multibyte locales - "
                 "use mbschr if you care about internationalization");
#endif

/* Find the first occurrence of C in S or the final NUL byte.  */
#if @GNULIB_STRCHRNUL@
# if @REPLACE_STRCHRNUL@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   define strchrnul rpl_strchrnul
#  endif
_GL_FUNCDECL_RPL (strchrnul, char *, (const char *__s, int __c_in)
                                     _GL_ATTRIBUTE_PURE
                                     _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (strchrnul, char *,
                  (const char *str, int ch));
# else
#  if ! @HAVE_STRCHRNUL@
_GL_FUNCDECL_SYS (strchrnul, char *, (char const *__s, int __c_in)
                                     _GL_ATTRIBUTE_PURE
                                     _GL_ARG_NONNULL ((1)));
#  endif
  /* On some systems, this function is defined as an overloaded function:
       extern "C++" { const char * std::strchrnul (const char *, int); }
       extern "C++" { char * std::strchrnul (char *, int); }  */
_GL_CXXALIAS_SYS_CAST2 (strchrnul,
                        char *, (char const *__s, int __c_in),
                        char const *, (char const *__s, int __c_in));
# endif
# if ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 10) && !defined __UCLIBC__) \
     && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
_GL_CXXALIASWARN1 (strchrnul, char *, (char *__s, int __c_in));
_GL_CXXALIASWARN1 (strchrnul, char const *, (char const *__s, int __c_in));
# else
_GL_CXXALIASWARN (strchrnul);
# endif
#elif defined GNULIB_POSIXCHECK
# undef strchrnul
# if HAVE_RAW_DECL_STRCHRNUL
_GL_WARN_ON_USE (strchrnul, "strchrnul is unportable - "
                 "use gnulib module strchrnul for portability");
# endif
#endif

/* Duplicate S, returning an identical malloc'd string.  */
#if @GNULIB_STRDUP@
# if @REPLACE_STRDUP@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef strdup
#   define strdup rpl_strdup
#  endif
_GL_FUNCDECL_RPL (strdup, char *, (char const *__s) _GL_ARG_NONNULL ((1)));
_GL_CXXALIAS_RPL (strdup, char *, (char const *__s));
# else
#  if defined __cplusplus && defined GNULIB_NAMESPACE && defined strdup
    /* strdup exists as a function and as a macro.  Get rid of the macro.  */
#   undef strdup
#  endif
#  if !(@HAVE_DECL_STRDUP@ || defined strdup)
_GL_FUNCDECL_SYS (strdup, char *, (char const *__s) _GL_ARG_NONNULL ((1)));
#  endif
_GL_CXXALIAS_SYS (strdup, char *, (char const *__s));
# endif
_GL_CXXALIASWARN (strdup);
#elif defined GNULIB_POSIXCHECK
# undef strdup
# if HAVE_RAW_DECL_STRDUP
_GL_WARN_ON_USE (strdup, "strdup is unportable - "
                 "use gnulib module strdup for portability");
# endif
#endif

/* Append no more than N characters from SRC onto DEST.  */
#if @GNULIB_STRNCAT@
# if @REPLACE_STRNCAT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef strncat
#   define strncat rpl_strncat
#  endif
_GL_FUNCDECL_RPL (strncat, char *, (char *dest, const char *src, size_t n)
                                   _GL_ARG_NONNULL ((1, 2)));
_GL_CXXALIAS_RPL (strncat, char *, (char *dest, const char *src, size