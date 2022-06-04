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
#ifndef LIBBASE64_H
#define LIBBASE64_H


#include <stdlib.h>

#define LIBBASE64_VERSION "1.0.0"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef CONFIG_LIBPOSIX
#include <libposix.h>

GEAR_API size_t b64e_size(size_t in_size);
GEAR_API size_t b64d_size(size_t in_size);
GEAR_API size_t base64_encode(char* target, const void *source, size_t bytes);
GEAR_API size_t base64_encode_url(char* target, const void *source, size_t bytes);
GEAR_API size_t base64_decode(void* target, const char *source, size_t bytes);

GEAR_API size_t base16_encode(char* target, const void *source, size_t bytes);
GEAR_API size_t base16_decode(void* target, const char *source, size_t bytes);

#else

size_t b64e_size(size_t in_size);
size_t b64d_size(size_t in_size);
size_t base64_encode(char* target, const void *source, size_t bytes);
size_t base64_encode_url(char* target, const void *source, size_t bytes);
size_t base64_decode(void* target, const char *source, size_t bytes);

size_t base16_encode(char* target, const void *source, size_t bytes);
size_t base16_decode(void* target, const char *source, size_t bytes);

#endif

#ifdef __cplusplus
}
#endif
#endif
