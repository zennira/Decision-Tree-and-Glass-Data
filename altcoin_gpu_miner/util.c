/*
 * Copyright 2011-2013 Con Kolivas
 * Copyright 2010 Jeff Garzik
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <jansson.h>
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef WIN32
#include <fcntl.h>
# ifdef __linux
#  include <sys/prctl.h>
# endif
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
#else
# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# include <mmsystem.h>
#endif

#include "miner.h"
#include "elist.h"
#include "compat.h"
#include "util.h"

#define DEFAULT_SOCKWAIT 60

bool successful_connect = false;
static void keep_sockalive(SOCKETTYPE fd)
{
	const int tcp_one = 1;
#ifndef WIN32
	const int tcp_keepidle = 45;
	const int tcp_keepintvl = 30;
	int flags = fcntl(fd, F_GETFL, 0);

	fcntl(fd, F_SETFL, O_NONBLOCK | flags);
#else
	u_long flags = 1;

	ioctlsocket(fd, FIONBIO, &flags);
#endif

	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&tcp_one, sizeof(tcp_one));
	if (!opt_delaynet)
#ifndef __linux
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&tcp_one, sizeof(tcp_one));
#else /* __linux */
		setsockopt(fd, SOL_TCP, TCP_NODELAY, (const void *)&tcp_one, sizeof(tcp_one));
	setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &tcp_one, sizeof(tcp_one));
	setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &tcp_keepidle, sizeof(tcp_keepidle));
	setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &tcp_keepintvl, sizeof(tcp_keepintvl));
#endif /* __linux */

#ifdef __APPLE_CC__
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &tcp_keepintvl, sizeof(tcp_keepintvl));
#endif /* __APPLE_CC__ */

}

struct tq_ent {
	void			*data;
	struct list_head	q_node;
};

#ifdef HAVE_LIBCURL
struct timeval nettime;

struct data_buffer {
	void		*buf;
	size_t		len;
};

struct upload_buffer {
	const void	*buf;
	size_t		len;
};

struct header_info {
	char		*lp_path;
	int		rolltime;
	char		*reason;
	char		*stratum_url;
	bool		hadrolltime;
	bool		canroll;
	bool		hadexpire;
};

static void databuf_free(struct data_buffer *db)
{
	if (!db)
		return;

	free(db->buf);

	memset(db, 0, sizeof(*db));
}

static size_t all_data_cb(const void *ptr, size_t size, size_t nmemb,
			  void *user_data)
{
	struct data_buffer *db = user_data;
	size_t len = size * nmemb;
	size_t oldlen, newlen;
	void *newmem;
	static const unsigned char zero = 0;

	oldlen = db->len;
	newlen = oldlen + len;

	newmem = realloc(db->buf, newlen + 1);
	if (!newmem)
		return 0;

	db->buf = newmem;
	db->len = newlen;
	memcpy(db->buf + oldlen, ptr, len);
	memcpy(db->buf + newlen, &zero, 1);	/* null terminate */

	return len;
}

static size_t upload_data_cb(void *ptr, size_t size, size_t nmemb,
			     void *user_data)
{
	struct upload_buffer *ub = user_data;
	unsigned int len = size * nmemb;

	if (len > ub->len)
		len = ub->len;

	if (len) {
		memcpy(ptr, ub->buf, len);
		ub->buf += len;
		ub->len -= len;
	}

	return len;
}

static size_t resp_hdr_cb(void *ptr, size_t size, size_t nmemb, void *user_data)
{
	struct header_info *hi = user_data;
	size_t remlen, slen, ptrlen = size * nmemb;
	char *rem, *val = NULL, *key = NULL;
	void *tmp;

	val = calloc(1, ptrlen);
	key = calloc(1, ptrlen);
	if (!key || !val)
		goto out;

	tmp = memchr(ptr, ':', ptrlen);
	if (!tmp || (tmp == ptr))	/* skip empty keys / blanks */
		goto out;
	slen = tmp - ptr;
	if ((slen + 1) == ptrlen)	/* skip key w/ no value */
		goto out;
	memcpy(key, ptr, slen);		/* store & nul term key */
	key[slen] = 0;

	rem = ptr + slen + 1;		/* trim value's leading whitespace */
	remlen = ptrlen - slen - 1;
	while ((remlen > 0) && (isspace(*rem))) {
		remlen--;
		rem++;
	}

	memcpy(val, rem, remlen);	/* store value, trim trailing ws */
	val[remlen] = 0;
	while ((*val) && (isspace(val[strlen(val) - 1])))
		val[strlen(val) - 1] = 0;

	if (!*val)			/* skip blank value */
		goto out;

	if (opt_protocol)
		applog(LOG_DEBUG, "HTTP hdr(%s): %s", key, val);

	if (!strcasecmp("X-Roll-Ntime", key)) {
		hi->hadrolltime = true;
		if (!strncasecmp("N", val, 1))
			applog(LOG_DEBUG, "X-Roll-Ntime: N found");
		else {
			hi->canroll = true;

			/* Check to see if expire= is supported and if not, set
			 * the rolltime to the default scantime */
			if (strlen(val) > 7 && !strncasecmp("expire=", val, 7)) {
				sscanf(val + 7, "%d", &hi->rolltime);
				hi->hadexpire = true;
			} else
				hi->rolltime = opt_scantime;
			applog(LOG_DEBUG, "X-Roll-Ntime expiry set to %d", hi->rolltime);
		}
	}

	if (!strcasecmp("X-Long-Polling", key)) {
		hi->lp_path = val;	/* steal me