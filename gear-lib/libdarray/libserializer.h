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
#ifndef LIB_SERIALIZER_H
#define LIB_SERIALIZER_H

#include "libdarray.h"

#ifdef __cplusplus
extern "C" {
#endif

struct serializer {
    void *data;
    size_t  (*read)(void *, void *, size_t);
    size_t  (*write)(void *, const void *, size_t);
    size_t  (*getpos)(void *);
    void    (*free)(void *);
};

int serializer_array_init(struct serializer *s);
void serializer_array_deinit(struct serializer *s);
int serializer_array_get_data(struct serializer *s, uint8_t **output, size_t *size);
void serializer_array_reset(struct serializer *s);

int serializer_file_init(struct serializer *s, const char *path);
void serializer_file_deinit(struct serializer *s);

size_t s_read(struct serializer *s, void *data, size_t size);
size_t s_write(struct serializer *s, const void *data, size_t size);
size_t s_getpos(struct serializer *s);

void s_w8(struct serializer *s, uint8_t u8);
void s_wl16(struct serializer *s, uint16_t u16);
void s_wl24(struct serializer *s, uint32_t u24);
void s_wl32(struct serializer *s, uint32_t u32);
void s_wl64(struct serializer *s, uint64_t u64);
void s_wlf(struct serializer *s, float f);
void s_wld(struct serializer *s, double d);
void s_wb16(struct serializer *s, uint16_t u16);
void s_wb24(struct serializer *s, uint32_t u24);
void s_wb32(struct serializer *s, uint32_t u32);
void s_wb64(struct serializer *s, uint64_t u64);
void s_wbf(struct serializer *s, float f);
void s_wbd(struct serializer *s, double d);

#ifdef __cplusplus
}
#endif

#endif
