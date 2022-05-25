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
#include "libposix.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/param.h>

#ifndef PATH_SPLIT
#define PATH_SPLIT       '/'
#endif


int get_proc_name(char *name, size_t len)
{
    int i, ret;
    char proc_name[PATH_MAX];
    char *ptr = NULL;
    memset(proc_name, 0, sizeof(proc_name));
    if (-1 == readlink("/proc/self/exe", proc_name, sizeof(proc_name))) {
        fprintf(stderr, "readlink failed!\n");
        return -1;
    }
    ret = strlen(proc_name);
    for (i = ret, ptr = proc_name; i > 0; i--) {
        if (ptr[i] == PATH_SPLIT) {
            ptr+= i+1;
            break;
        }
    }
    if (i == 0) {
        fprintf(stderr, "proc path %s is invalid\n", proc_name);
        return -1;
    }
    if (ret-i > (int)len) {
        fprintf(stderr, "proc name length %d is larger than %d\n", ret-i, (int)len);
        return -1;
    }
    strncpy(name, ptr, ret - i);
    return 0;
}

#define UTF8_IGNORE_ERROR 0x01
#define UTF8_SKIP_BOM 0x02

#define _NXT 0x80
#define _SEQ2 0xc0
#define _SEQ3 0xe0
#define _SEQ4 0xf0
#define _SEQ5 0xf8
#define _SEQ6 0xfc

#define _BOM 0xfeff

static int wchar_forbidden(wchar_t sym)
{
    /* Surrogate pairs */
    if (sym >= 0xd800 && sym <= 0xdfff)
        return -1;
    return 0;
}

static int utf8_forbidden(unsigned char octet)
{
    switch (octet) {
    case 0xc0:
    case 0xc1:
    case 0xf5:
    case 0xff:
        return -1;
    }
    return 0;
}

size_t utf8_to_wchar(const char *in, size_t insize, wchar_t *out, size_t outsize, int flags)
{
    unsigned char *p, *lim;
    wchar_t *wlim, high;
    size_t n, total, i, n_bits;

    if (in == NULL || (outsize == 0 && out != NULL))
        return 0;

    total = 0;
    p = (unsigned char *)in;
    lim = (insize != 0) ? (p + insize) : (unsigned char *)-1;
    wlim = out + outsize;

    for (; p < lim; p += n) {
        if (!*p)
            break;
        if (utf8_forbidden(*p) != 0 && (flags & UTF8_IGNORE_ERROR) == 0)
            return 0;

        /*
         * Get number of bytes for one wide character.
         */
        n = 1; /* default: 1 byte. Used when skipping bytes. */
        if ((*p & 0x80) == 0)
            high = (wchar_t)*p;
        else if ((*p & 0xe0) == _SEQ2) {
            n = 2;
            high = (wchar_t)(*p & 0x1f);
        } else if ((*p & 0xf0) == _SEQ3) {
            n = 3;
            high = (wchar_t)(*p & 0x0f);
        } else if ((*p & 0xf8) == _SEQ4) {
            n = 4;
            high = (wchar_t)(*p & 0x07);
        } else if ((*p & 0xfc) == _SEQ5) {
            n = 5;
            high = (wchar_t)(*p & 0x03);
        } else if ((*p & 0xfe) == _SEQ6) {
            n = 6;
            high = (wchar_t)(*p & 0x01);
        } else {
            if ((flags & UTF8_IGNORE_ERROR) == 0)
                return 0;
            continue;
        }

        /* does the sequence header tell us truth about length? */
        if ((size_t)(lim - p) <= n - 1) {
            if ((flags & UTF8_IGNORE_ERROR) == 0)
                return 0;
            n = 1;
            continue; /* skip */
        }

        /*
         * Validate sequence.
         * All symbols must have higher bits set to 10xxxxxx
         */
        if (n > 1) {
            for (i = 1; i < n; i++) {
                if ((p[i] & 0xc0) != _NXT)
                    break;
            }
            if (i != n) {
                if ((flags & UTF8_IGNORE_ERROR) == 0)
                    return 0;
                n = 1;
                continue; /* skip */
            }
        }

        total++;
        if (out == NULL)
            continue;
        if (out >= wlim)
            return 0; /* no space left */

        *out = 0;
        n_bits = 0;
        for (i = 1; i < n; i++) {
            *out |= (wchar_t)(p[n - i] & 0x3f) << n_bits;
            n_bits += 6; /* 6 low bits in every byte */
        }
        *out |= high << n_bits;

        if (wchar_forbidden(*out) != 0) {
            if ((flags & UTF8_IGNORE_ERROR) == 0)
                return 0; /* forbidden character */
            else {
                total--;
                out--;
            }
        } else if (*out == _BOM && (flags & UTF8_SKIP_BOM) != 0) {
            total--;
            out--;
        }
        out++;
    }
    return total;
}

size_t wchar_to_utf8(const wchar_t *in, size_t insize, char *out, size_t outsize, int flags)
{
    wchar_t *w, *wlim, ch = 0;
    unsigned char *p, *lim, *oc;
    size_t total, n;

    if (in == NULL || (outsize == 0 && out != NULL))
        return 0;

    w = (wchar_t *)in;
    wlim = (insize != 0) ? (w + insize) : (wchar_t *)-1;
    p = (unsigned char *)out;
    lim = p + outsize;
    total = 0;

    for (; w < wlim; w++) {
        if (!*w)
            break;

        if (wchar_forbidden(*w) != 0) {
            if ((flags & UTF8_IGNORE_ERROR) == 0)
                return 0;
            else
                continue;
        }

        if (*w == _BOM && (flags & UTF8_SKIP_BOM) != 0)
            continue;

        if (*w < 0) {
            if ((flags & UTF8_IGNORE_ERROR) == 0)
                return 0;
            continue;
        } else if (*w <= 0x0000007f)
            n = 1;
        else if (*w <= 0x000007ff)
            n = 2;
        else if (*w <= 0x0000ffff)
            n = 3;
        else if (*w <= 0x001fffff)
            n = 4;
        else if (*w <= 0x03ffffff)
            n = 5;
        else /* if (*w <= 0x7fffffff) */
            n = 6;

        total += n;

        if (out == NULL)
            continue;

        if ((size_t)(lim - p) <= n - 1)
            return 0; /* no space left */

        ch = *w;
        oc = (unsigned char *)&ch;
        switch (n) {
        case 1:
            *p = oc[0];
            break;
        case 2:
            p[1] = _NXT | (oc[0] & 0x3f);
            p[0] = _SEQ2 | (oc[0] >> 6) | ((oc[1] & 0x07) << 2);
            break;
        case 3:
            p[2] = _NXT | (oc[0] & 0x3f);
            p[1] = _NXT | (oc[0] >> 6) | ((oc[1] & 0x0f) << 2);
            p[0] = _SEQ3 | ((oc[1] & 0xf0) >> 4);
            break;
        case 4:
            p[3] = _NXT | (oc[0] & 0x3f);
            p[2] = _NXT | (oc[0] >> 6) | ((oc[1] & 0x0f) << 2);
            p[1] = _NXT | ((oc[1] & 0xf0) >> 4) |
                    ((oc[2] & 0x03) << 4);
            p[0] = _SEQ4 | ((oc[2] & 0x1f) >> 2);
            break;
        case 5:
            p[4] = _NXT | (oc[0] & 0x3f);
            p[3] = _NXT | (oc[0] >> 6) | ((oc[1] & 0x0f) << 2);
            p[2] = _NXT | ((oc[1] & 0xf0) >> 4) |
                    ((oc[2] & 0x03) << 4);
            p[1] = _NXT | (oc[2] >> 2);
            p[0] = _SEQ5 | (oc[3] & 0x03);
            break;
        case 6:
            p[5] = _NXT | (oc[0] & 0x3f);
            p[4] = _NXT | (oc[0] >> 6) | ((oc[1] & 0x0f) << 2);
            p[3] = _NXT | (oc[1] >> 4) | ((oc[2] & 0x03) << 4);
            p[2] = _NXT | (oc[2] >> 2);
            p[1] = _NXT | (oc[3] & 0x3f);
            p[0] = _SEQ6 | ((oc[3] & 0x40) >> 6);
            break;
        }

        /*
         * NOTE: do not check here for forbidden UTF-8 characters.
         * They cannot appear here because we do proper conversion.
         */

        p += n;
    }

    return total;
}
