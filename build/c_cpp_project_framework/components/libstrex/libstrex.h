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
#ifndef LIBSTREX_H
#define LIBSTREX_H

#include <libposix.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @strtrim remove white-space characters of whole string.
 *  In the "C" and "POSIX" locales, these are: space, form-feed ('\f'),
 *  newline ('\n'), carriage return ('\r'),  horizontal tab ('\t'),
 *  and vertical tab ('\v').
 *
 * @param  s: input string to be trimed
 * @return trimed string
 */
char *strtrim(char *s);


/**
 * @externtion of strncat - concatenate two strings
 */
size_t strlcat(char *dst, const char *src, size_t maxlen);
size_t strlncat(char *dst, size_t len, const char *src, size_t maxlen);

/**
 * @strlncat externtion of strncpy - copy a string
 */
size_t strlcpy(char *dst, const char *src, size_t len);
size_t strlncpy(char *dst, size_t len, const char *src, size_t n);


/**
 * @externtion of string convert uppercase or lowercase
 */
char *strtoupper(char *dst, char *src, size_t n);
char *strtolower(char *dst, char *src, size_t n);

int strhex2bin(char ch);

GEAR_API size_t base64_encode(char* target, const void *source, size_t bytes);
GEAR_API size_t base64_encode_url(char* target, const void *source, size_t bytes);
GEAR_API size_t base64_decode(void* target, const char *source, size_t bytes);

GEAR_API size_t base16_encode(char* target, const void *source, size_t bytes);
GEAR_API size_t base16_decode(void* target, const char *source, size_t bytes);

#ifdef __cplusplus
}
#endif
#endif
