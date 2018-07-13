/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef URI_PARSE_H
#define URI_PARSE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//e.g: http://usr:pwd@host:port/path?query#fragment
struct uri_t
{
    char* scheme;
    char* userinfo;
    char* host;
    int port;
    char* path;
    char* query;
    char* fragment;
};

int uri_parse(struct uri_t *uri, char* str, size_t len);

struct uri_query_t
{
    const char* name;
    int n_name;

    const char* value;
    int n_value;
};

int uri_query(const char* query, const char* end, struct uri_query_t** items);
void uri_query_free(struct uri_query_t** items);

int url_encode(const char* source, int srcBytes, char* target, int tgtBytes);
int url_decode(const char* source, int srcBytes, char* target, int tgtBytes);

#ifdef __cplusplus
}
#endif
#endif
