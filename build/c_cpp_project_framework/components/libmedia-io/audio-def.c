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

struct sample_format_name {
    enum sample_format format;
    char name[32];
};

static struct sample_format_name sample_fmt_tbl[] = {
    {SAMPLE_FORMAT_NONE,             "SAMPLE_FORMAT_NONE"},
    {SAMPLE_FORMAT_PCM_U8,           "PCM_U8"},
    {SAMPLE_FORMAT_PCM_ALAW,         "PCM_ALAW"},
    {SAMPLE_FORMAT_PCM_ULAW,         "PCM_ULAW"},
    {SAMPLE_FORMAT_PCM_S16LE,        "PCM_S16LE"},
    {SAMPLE_FORMAT_PCM_S16BE,        "PCM_S16BE"},
    {SAMPLE_FORMAT_PCM_S24LE,        "PCM_S24LE"},
    {SAMPLE_FORMAT_PCM_S24BE,        "PCM_S24BE"},
    {SAMPLE_FORMAT_PCM_S32LE,        "PCM_S32LE"},
    {SAMPLE_FORMAT_PCM_S32BE,        "PCM_S32BE"},
    {SAMPLE_FORMAT_PCM_S24_32LE,     "PCM_S24_32LE"},
    {SAMPLE_FORMAT_PCM_S24_32BE,     "PCM_S24_32BE"},
    {SAMPLE_FORMAT_PCM_F32LE,        "PCM_F32LE"},
    {SAMPLE_FORMAT_PCM_F32BE,        "PCM_F32BE"},
    {SAMPLE_FORMAT_PCM_S16LE_PLANAR, "PCM_S16LE_PLANAR"},
    {SAMPLE_FORMAT_PCM_S16BE_PLANAR, "PCM_S16BE_PLANAR"},
    {SAMPLE_FORMAT_PCM_S24LE_PLANAR, "PCM_S24LE_PLANAR"},
    {SAMPLE_FORMAT_PCM_S32LE_PLANAR, "PCM_S32LE_PLANAR"},
    {SAMPLE_FORMAT_PCM_MAX,          "SAMPLE_FORMAT_PCM_MAX"},
};

const char *sample_format_to_string(enum sample_format fmt)
{
    fmt = (fmt > SAMPLE_FORMAT_PCM_MAX) ? SAMPLE_FORMAT_PCM_MAX : fmt;
    return sample_fmt_tbl[fmt].name;
}

enum sample_format sample_string_to_format(const char *name)
{
    int i;
    if (!name) {
        return SAMPLE_FORMAT_NONE;
    }
    for (i = 0; i < SAMPLE_FORMAT_PCM_MAX; i++) {
        if (!strncasecmp(name, sample_fmt_tbl[i].name, sizeof(sample_fmt_tbl[i].name))) {
            return sample_fmt_tbl[i].format;
        }
    }
    return SAMPLE_FORMAT_NONE;
}

void audio_producer_dump(struct audio_producer *as)
{

}

struct audio_packet *audio_packet_create(enum media_mem_type type, void *data, size_t len)
{
    struct audio_packet *ap;
    ap = calloc(1, sizeof(struct audio_packet));
    if (!ap) {
        return NULL;
    }
    ap->mem_type = type;
    switch (type) {
    case MEDIA_MEM_DEEP:
        ap->data = memdup(data, len);
        ap->size = len;
        break;
    case MEDIA_MEM_SHALLOW:
        ap->data = data;
        ap->size = len;
        break;
    default:
        printf("%s invalid type!\n", __func__);
        break;
    }

    return ap;
}

void audio_packet_destroy(struct audio_packet *ap)
{
    if (ap) {
#if 0
        if (ap->data) {
            free(ap->data);
        }
#endif
        free(ap);
    }
}

struct audio_packet *audio_packet_copy(struct audio_packet *dst, const struct audio_packet *src, media_mem_type_t type)
{
    if (!dst || !src) {
        printf("%s invalid paramenters!\n", __func__);
        return NULL;
    }
    switch (dst->mem_type) {
    case MEDIA_MEM_SHALLOW:
        dst->data = src->data;
        break;
    case MEDIA_MEM_DEEP:
        if (!dst->data) {
            dst->data = calloc(1, src->size);
        }
        memcpy(dst->data, src->data, src->size);
        break;
    }
    dst->size = src->size;
    dst->pts  = src->pts;
    dst->dts  = src->dts;
    dst->track_idx = src->track_idx;
    dst->encoder = src->encoder;
    return dst;
}

void audio_encoder_dump(struct audio_encoder *ae)
{

}
