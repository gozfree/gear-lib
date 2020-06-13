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
#include "libmedia-io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

const char *sample_format_name(enum sample_format format)
{
    switch (format) {
    case SAMPLE_FORMAT_U8BIT:
        return "u8";
    case SAMPLE_FORMAT_16BIT:
        return "s16le";
    case SAMPLE_FORMAT_32BIT:
        return "s32le";
    case SAMPLE_FORMAT_FLOAT:
        return "f32le";
    case SAMPLE_FORMAT_U8BIT_PLANAR:
    case SAMPLE_FORMAT_16BIT_PLANAR:
    case SAMPLE_FORMAT_32BIT_PLANAR:
    case SAMPLE_FORMAT_FLOAT_PLANAR:
    case SAMPLE_FORMAT_UNKNOWN:
    case SAMPLE_FORMAT_CNT:;
    }

    return "None";
}

struct audio_packet *audio_packet_create(void *data, size_t len)
{
    struct audio_packet *ap;
    ap = calloc(1, sizeof(struct audio_packet));
    if (!ap) {
        return NULL;
    }
    ap->data = data;
    ap->size = len;
    return ap;
}

void audio_packet_destroy(struct audio_packet *ap)
{
    if (ap) {
        if (ap->data) {
            free(ap->data);
        }
        free(ap);
    }
}
