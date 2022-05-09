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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libstrex.h"

char *strtrim(char *s)
{
    char *p = s;
    char *q = s;

    while (*p != '\0') {
        if (!isspace(*p)) {
            *q = *p;
            q++;
        }
        p++;
    }
    *(q++) = '\0';

    return s;
}

size_t strlncat(char *dst, size_t len, const char *src, size_t maxlen)
{
    size_t slen, dlen, rlen, ncpy;

    slen = strnlen(src, maxlen);
    dlen = strnlen(dst, len);

    if (dlen < len) {
          rlen = len - dlen;
          ncpy = slen < rlen ? slen : (rlen - 1);
          memcpy(dst + dlen, src, ncpy);
          dst[dlen + ncpy] = '\0';
    }

    return (len > slen + dlen) ? (slen + dlen) : -1;
}

size_t strlcat(char *dst, const char *src, size_t maxlen)
{
    return strlncat(dst, maxlen, src, (size_t) -1);
}

size_t strlncpy(char *dst, size_t len, const char *src, size_t n)
{
    size_t slen;
    size_t ncpy;

    slen = strnlen(src, n);

    if (len > 0) {
        ncpy = slen < len ? slen : (len - 1);
        memcpy(dst, src, ncpy);
        dst[ncpy] = '\0';
    }

    return (len > slen) ? slen : -1;
}

size_t strlcpy(char *dst, const char *src, size_t len)
{
    return strlncpy(dst, len, src, (size_t) -1);
}

