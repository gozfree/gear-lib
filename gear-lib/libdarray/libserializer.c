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
#include "libdarray.h"
#include "libserializer.h"

struct array_data {
	DARRAY(uint8_t) bytes;
};

static size_t array_write(void *param, const void *data, size_t size)
{
    struct array_data *da = param;
    da_push_back_array(da->bytes, data, size);
    return size;
}

static size_t array_getpos(void *param)
{
    struct array_data *da = param;
    return (size_t)da->bytes.num;
}

static void array_free(void *param)
{
    struct array_data *da = param;
    da_free(da->bytes);
}

int serializer_array_init(struct serializer *s)
{
    struct array_data *data = calloc(1, sizeof(struct array_data));
    if (!data) {
        return -1;
    }
    memset(s, 0, sizeof(struct serializer));
    da_init(data->bytes);
    s->data   = data;
    s->read   = NULL;
    s->write  = array_write;
    s->getpos = array_getpos;
    s->free   = array_free;
    return 0;
}

int serializer_array_get_data(struct serializer *s, uint8_t **output, size_t *size)
{
    struct array_data *data = s->data;
    *output = data->bytes.array;
    *size = data->bytes.num;
    return 0;
}

void serializer_array_reset(struct serializer *s)
{
    struct array_data *data = s->data;
    s->free(data);
    memset(data, 0, sizeof(struct array_data));
}

void serializer_array_deinit(struct serializer *s)
{
    if (s->data)
        s->free(s->data);
    free(s->data);
    memset(s, 0, sizeof(struct serializer));
}

static size_t file_read(void *file, void *data, size_t size)
{
    return fread(data, 1, size, file);
}

static size_t file_write(void *file, const void *data, size_t size)
{
    return fwrite(data, 1, size, file);
}

static size_t file_getpos(void *file)
{
    return ftell(file);
}

int serializer_file_init(struct serializer *s, const char *path)
{
    memset(s, 0, sizeof(struct serializer));
    s->data = fopen(path, "rb");
    if (!s->data)
        return -1;

    s->read = file_read;
    s->write = file_write;
    s->getpos = file_getpos;
    return 0;
}

void serializer_file_deinit(struct serializer *s)
{
    if (s->data)
        fclose(s->data);
    memset(s, 0, sizeof(struct serializer));
}

size_t s_read(struct serializer *s, void *data, size_t size)
{
    if (s && s->read && data && size)
        return s->read(s->data, (void *)data, size);
    return 0;
}

size_t s_write(struct serializer *s, const void *data, size_t size)
{
    if (s && s->write && data && size)
        return s->write(s->data, (void *)data, size);
    return 0;
}

size_t s_getpos(struct serializer *s)
{
    if (s && s->getpos)
        return s->getpos(s->data);
    return -1;
}

void s_w8(struct serializer *s, uint8_t u8)
{
    s_write(s, &u8, sizeof(uint8_t));
}

void s_wl16(struct serializer *s, uint16_t u16)
{
    s_w8(s, (uint8_t)u16);
    s_w8(s, u16 >> 8);
}

void s_wl24(struct serializer *s, uint32_t u24)
{
    s_w8(s, (uint8_t)u24);
    s_wl16(s, (uint16_t)(u24 >> 8));
}

void s_wl32(struct serializer *s, uint32_t u32)
{
    s_wl16(s, (uint16_t)u32);
    s_wl16(s, (uint16_t)(u32 >> 16));
}

void s_wl64(struct serializer *s, uint64_t u64)
{
    s_wl32(s, (uint32_t)u64);
    s_wl32(s, (uint32_t)(u64 >> 32));
}

void s_wlf(struct serializer *s, float f)
{
    s_wl32(s, *(uint32_t *)&f);
}

void s_wld(struct serializer *s, double d)
{
    s_wl64(s, *(uint64_t *)&d);
}

void s_wb16(struct serializer *s, uint16_t u16)
{
    s_w8(s, u16 >> 8);
    s_w8(s, (uint8_t)u16);
}

void s_wb24(struct serializer *s, uint32_t u24)
{
    s_wb16(s, (uint16_t)(u24 >> 8));
    s_w8(s, (uint8_t)u24);
}

void s_wb32(struct serializer *s, uint32_t u32)
{
    s_wb16(s, (uint16_t)(u32 >> 16));
    s_wb16(s, (uint16_t)u32);
}

void s_wb64(struct serializer *s, uint64_t u64)
{
    s_wb32(s, (uint32_t)(u64 >> 32));
    s_wb32(s, (uint32_t)u64);
}

void s_wbf(struct serializer *s, float f)
{
    s_wb32(s, *(uint32_t *)&f);
}

void s_wbd(struct serializer *s, double d)
{
    s_wb64(s, *(uint64_t *)&d);
}
