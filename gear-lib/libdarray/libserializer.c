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
#include "libdarray.h"
#include "libserializer.h"

static size_t array_output_write(void *param, const void *data, size_t size)
{
	struct darray_data *output = param;
	da_push_back_array(output->bytes, data, size);
	return size;
}

static int64_t array_output_get_pos(void *param)
{
	struct darray_data *data = param;
	return (int64_t)data->bytes.num;
}

void serializer_init(struct serializer *s, struct darray_data *data)
{
	memset(s, 0, sizeof(struct serializer));
	memset(data, 0, sizeof(struct darray_data));
	s->data = data;
	s->write = array_output_write;
	s->get_pos = array_output_get_pos;
}

void serializer_data_free(struct darray_data *data)
{
	da_free(data->bytes);
}

size_t s_read(struct serializer *s, void *data, size_t size)
{
	if (s && s->read && data && size)
		return s->read(s->data, (void *)data, size);
	return 0;
}

size_t s_write(struct serializer *s, const void *data,
			     size_t size)
{
	if (s && s->write && data && size)
		return s->write(s->data, (void *)data, size);
	return 0;
}

size_t serialize(struct serializer *s, void *data, size_t len)
{
	if (s) {
		if (s->write)
			return s->write(s->data, data, len);
		else if (s->read)
			return s->read(s->data, data, len);
	}

	return 0;
}

int64_t serializer_seek(struct serializer *s, int64_t offset,
				      enum serialize_seek_type seek_type)
{
	if (s && s->seek)
		return s->seek(s->data, offset, seek_type);
	return -1;
}

int64_t serializer_get_pos(struct serializer *s)
{
	if (s && s->get_pos)
		return s->get_pos(s->data);
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
