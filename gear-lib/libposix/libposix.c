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
#define _CRT_SECURE_NO_WARNINGS /* Disable safety warning for mbstowcs() */
#include "libposix.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>

void *memdup(const void *src, size_t len)
{
    void *dst = NULL;
    if (len == 0) {
        return NULL;
    }
    dst = calloc(1, len);
    if (LIKELY(dst != NULL)) {
        memcpy(dst, src, len);
    }
    return dst;
}

struct iovec *iovec_create(size_t len)
{
    struct iovec *vec = calloc(1, sizeof(struct iovec));
    if (LIKELY(vec != NULL)) {
        vec->iov_len = len;
        vec->iov_base = calloc(1, len);
        if (UNLIKELY(vec->iov_base == NULL)) {
            free(vec);
            vec = NULL;
        }
    }
    return vec;
}

void iovec_destroy(struct iovec *vec)
{
    if (LIKELY(vec != NULL)) {
        /* free(NULL) do nop */
        free(vec->iov_base);
        free(vec);
    }
}


/**
 * Fast little endian check
 * NOTE: not applicable for PDP endian
 */
bool is_little_endian(void)
{
    uint16_t x = 0x01;
    return *((uint8_t *) &x);
}

size_t mbs_to_wcs(const char *str, size_t len, wchar_t *dst, size_t dst_size)
{
    size_t out_len;

    if (!str)
        return 0;

    out_len = dst ? (dst_size - 1) : mbstowcs(NULL, str, len);
    if (dst) {
        if (!dst_size)
            return 0;
        if (out_len)
            out_len = mbstowcs(dst, str, out_len + 1);
        dst[out_len] = 0;
    }
    return out_len;
}

size_t wcs_to_mbs(const wchar_t *str, size_t len, char *dst, size_t dst_size)
{
    size_t out_len;

    if (!str)
        return 0;

    out_len = dst ? (dst_size - 1) : wcstombs(NULL, str, len);
    if (dst) {
        if (!dst_size)
            return 0;
        if (out_len)
            out_len = wcstombs(dst, str, out_len + 1);

        dst[out_len] = 0;
    }
    return out_len;
}

size_t utf8_to_wcs(const char *str, size_t len, wchar_t *dst, size_t dst_size)
{
    size_t in_len;
    size_t out_len;

    if (!str)
        return 0;

    in_len = len ? len : strlen(str);
    out_len = dst ? (dst_size - 1) : utf8_to_wchar(str, in_len, NULL, 0, 0);

    if (dst) {
        if (!dst_size)
            return 0;
        if (out_len)
            out_len = utf8_to_wchar(str, in_len, dst, out_len + 1, 0);
        dst[out_len] = 0;
    }
    return out_len;
}

size_t wcs_to_utf8(const wchar_t *str, size_t len, char *dst, size_t dst_size)
{
    size_t in_len;
    size_t out_len;

    if (!str)
        return 0;

    in_len = (len != 0) ? len : wcslen(str);
    out_len = dst ? (dst_size - 1) : wchar_to_utf8(str, in_len, NULL, 0, 0);
    if (dst) {
        if (!dst_size)
            return 0;
        if (out_len)
            out_len = wchar_to_utf8(str, in_len, dst, out_len + 1, 0);
        dst[out_len] = 0;
    }
    return out_len;
}

size_t mbs_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr)
{
    if (str) {
        size_t out_len = mbs_to_wcs(str, len, NULL, 0);

        *pstr = malloc((out_len + 1) * sizeof(wchar_t));
        return mbs_to_wcs(str, len, *pstr, out_len + 1);
    } else {
        *pstr = NULL;
        return 0;
    }
}

size_t wcs_to_mbs_ptr(const wchar_t *str, size_t len, char **pstr)
{
    if (str) {
        size_t out_len = wcs_to_mbs(str, len, NULL, 0);

        *pstr = malloc((out_len + 1) * sizeof(char));
        return wcs_to_mbs(str, len, *pstr, out_len + 1);
    } else {
        *pstr = NULL;
        return 0;
    }
}

size_t utf8_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr)
{
    if (str) {
        size_t out_len = utf8_to_wcs(str, len, NULL, 0);
        *pstr = malloc((out_len + 1) * sizeof(wchar_t));
        return utf8_to_wcs(str, len, *pstr, out_len + 1);
    } else {
        *pstr = NULL;
        return 0;
    }
}

size_t wcs_to_utf8_ptr(const wchar_t *str, size_t len, char **pstr)
{
    if (str) {
        size_t out_len = wcs_to_utf8(str, len, NULL, 0);
        *pstr = malloc((out_len + 1) * sizeof(char));
        return wcs_to_utf8(str, len, *pstr, out_len + 1);
    } else {
        *pstr = NULL;
        return 0;
    }
}

size_t utf8_to_mbs_ptr(const char *str, size_t len, char **pstr)
{
    char *dst = NULL;
    size_t out_len = 0;

    if (str) {
        wchar_t *wstr = NULL;
        size_t wlen = utf8_to_wcs_ptr(str, len, &wstr);
        out_len = wcs_to_mbs_ptr(wstr, wlen, &dst);
        free(wstr);
    }
    *pstr = dst;
    return out_len;
}

size_t mbs_to_utf8_ptr(const char *str, size_t len, char **pstr)
{
    char *dst = NULL;
    size_t out_len = 0;

    if (str) {
        wchar_t *wstr = NULL;
        size_t wlen = mbs_to_wcs_ptr(str, len, &wstr);
        out_len = wcs_to_utf8_ptr(wstr, wlen, &dst);
        free(wstr);
    }
    *pstr = dst;
    return out_len;
}

