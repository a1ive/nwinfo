/*
 * Copyright 2024  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIBCPUID_CTYPE_H__
#define __LIBCPUID_CTYPE_H__
/*
 * This file contains ctype.h function declarations for OS where this header cannot be included
 */

#if defined(__OpenBSD__)
/* On OpenBSD, _common_bits_t/_intel_bits_t/_amd_bits_t from libcpuid_internal.h conflict with ctype.h */
int	isalnum(int);
int	isalpha(int);
int	iscntrl(int);
int	isdigit(int);
int	isgraph(int);
int	islower(int);
int	isprint(int);
int	ispunct(int);
int	isspace(int);
int	isupper(int);
int	isxdigit(int);
int	tolower(int);
int	toupper(int);

#  if __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __POSIX_VISIBLE > 200112 \
    || __XPG_VISIBLE > 600
int	isblank(int);
#  endif

#  if __BSD_VISIBLE || __XPG_VISIBLE
int	isascii(int);
int	toascii(int);
int	_tolower(int);
int	_toupper(int);
#  endif /* __BSD_VISIBLE || __XPG_VISIBLE */

#  if __POSIX_VISIBLE >= 200809
int	isalnum_l(int, locale_t);
int	isalpha_l(int, locale_t);
int	isblank_l(int, locale_t);
int	iscntrl_l(int, locale_t);
int	isdigit_l(int, locale_t);
int	isgraph_l(int, locale_t);
int	islower_l(int, locale_t);
int	isprint_l(int, locale_t);
int	ispunct_l(int, locale_t);
int	isspace_l(int, locale_t);
int	isupper_l(int, locale_t);
int	isxdigit_l(int, locale_t);
int	tolower_l(int, locale_t);
int	toupper_l(int, locale_t);
#  endif
#else
#  include <ctype.h>
#endif

#endif /* __LIBCPUID_CTYPE_H__ */
