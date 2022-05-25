/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
