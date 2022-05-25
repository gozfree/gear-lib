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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define ALIGNMENT 32
#define ALIGN_SIZE(size, align) (((size) + (align - 1)) & (~(align - 1)))

#if defined _ISOC11_SOURCE || __USE_ISOC11 || defined __USE_ISOCXX11 || __ISO_C_VISIBLE >= 2011
/*
 * void *aligned_alloc(size_t, size_t) defined in stdlib.h
 */
#define aligned_free    free
#else
static void *aligned_alloc(size_t alignment, size_t size)
{
    long diff;
    void *ptr = malloc(size + alignment);
    if (ptr) {
        diff = ((~(long)ptr) & (alignment - 1)) + 1;
        ptr = (char *)ptr + diff;
        ((char *)ptr)[-1] = (char)diff;
    }
    return ptr;
}

static void aligned_free(void *ptr)
{
	if (ptr)
		free((char *)ptr - ((char *)ptr)[-1]);
}
#endif

struct pixel_format_name {
    enum pixel_format format;
    char name[32];
};

struct video_codec_type_name {
    enum video_codec_type type;
    char name[32];
};

static struct pixel_format_name pxlfmt_tbl[] = {
    {PIXEL_FORMAT_NONE, "PIXEL_FORMAT_NONE"},
    {PIXEL_FORMAT_I420, "I420"},
    {PIXEL_FORMAT_NV12, "NV12"},
    {PIXEL_FORMAT_YVYU, "YVYU"},
    {PIXEL_FORMAT_YUY2, "YUY2"},
    {PIXEL_FORMAT_UYVY, "UYVY"},
    {PIXEL_FORMAT_RGBA, "RGBA"},
    {PIXEL_FORMAT_BGRA, "BGRA"},
    {PIXEL_FORMAT_BGRX, "BGRX"},
    {PIXEL_FORMAT_Y800, "Y800"},
    {PIXEL_FORMAT_I444, "I444"},
    {PIXEL_FORMAT_RGB3, "RGB3"},
    {PIXEL_FORMAT_BGR3, "BGR3"},
    {PIXEL_FORMAT_I422, "I422"},
    {PIXEL_FORMAT_I40A, "I40A"},
    {PIXEL_FORMAT_I42A, "I42A"},
    {PIXEL_FORMAT_YUVA, "YUVA"},
    {PIXEL_FORMAT_AYUV, "AYUV"},
    {PIXEL_FORMAT_JPEG, "JPEG"},
    {PIXEL_FORMAT_MJPG, "MJPG"},
    {PIXEL_FORMAT_MAX,  "PIXEL_FORMAT_MAX"},
};

static struct video_codec_type_name video_codec_tbl[] = {
    {VIDEO_CODEC_NONE, "VIDEO_CODEC_NONE"},
    {VIDEO_CODEC_H264, "H264"},
    {VIDEO_CODEC_H265, "H265"},
    {VIDEO_CODEC_MAX, "VIDEO_CODEC_MAX"},
};

enum pixel_format pixel_string_to_format(const char *name)
{
    int i;
    if (!name) {
        return PIXEL_FORMAT_NONE;
    }
    for (i = 0; i < PIXEL_FORMAT_MAX; i++) {
        if (!strncasecmp(name, pxlfmt_tbl[i].name, sizeof(pxlfmt_tbl[i].name))) {
            return pxlfmt_tbl[i].format;
        }
    }
    return PIXEL_FORMAT_NONE;
}

const char *pixel_format_to_string(enum pixel_format fmt)
{
    fmt = (fmt > PIXEL_FORMAT_MAX) ? PIXEL_FORMAT_MAX : fmt;
    return pxlfmt_tbl[fmt].name;
}

enum video_codec_type video_codec_string_to_type(const char *name)
{
    int i;
    if (!name) {
        return VIDEO_CODEC_NONE;
    }
    for (i = 0; i < VIDEO_CODEC_MAX; i++) {
        if (!strncasecmp(name, video_codec_tbl[i].name, sizeof(video_codec_tbl[i].name))) {
            return video_codec_tbl[i].type;
        }
    }
    return VIDEO_CODEC_NONE;
}

const char *video_codec_type_to_string(enum video_codec_type type)
{
    type = (type > VIDEO_CODEC_MAX) ? VIDEO_CODEC_MAX : type;
    return video_codec_tbl[type].name;
}

int video_frame_init(struct video_frame *frame, enum pixel_format format,
                uint32_t width, uint32_t height, media_mem_type_t mem_type)
{
    size_t size;

    if (format == PIXEL_FORMAT_NONE || width == 0 || height == 0) {
        printf("%s:%d invalid paramenters!\n", __func__, __LINE__);
        return -1;
    }

    memset(frame, 0, sizeof(struct video_frame));
    frame->format = format;
    frame->width = width;
    frame->height = height;
    frame->mem_type = mem_type;

    switch (format) {
    case PIXEL_FORMAT_I420:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[1] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[2] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
           frame->data[0] = (uint8_t *)aligned_alloc(ALIGNMENT, size);
           frame->data[1] = (uint8_t *)frame->data[0] + frame->plane_offsets[1];
           frame->data[2] = (uint8_t *)frame->data[0] + frame->plane_offsets[2];
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        frame->planes = 3;
        break;
    case PIXEL_FORMAT_NV12:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[1] = size;
        size += (width / 2) * (height / 2) * 2;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
            frame->data[1] = (uint8_t *)frame->data[0] + frame->plane_offsets[1];
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        frame->planes = 2;
        break;
    case PIXEL_FORMAT_Y800:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
        }
        frame->linesize[0] = width;
        frame->planes = 1;
        break;
    case PIXEL_FORMAT_YVYU:
    case PIXEL_FORMAT_YUY2:
    case PIXEL_FORMAT_UYVY:
        size = width * height * 2;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
        }
        frame->linesize[0] = width * 2;
        frame->planes = 1;
        break;
    case PIXEL_FORMAT_RGBA:
    case PIXEL_FORMAT_BGRA:
    case PIXEL_FORMAT_BGRX:
    case PIXEL_FORMAT_AYUV:
        size = width * height * 4;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
        }
        frame->linesize[0] = width * 4;
        frame->planes = 1;
        break;
    case PIXEL_FORMAT_I444:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size * 3);
            frame->data[1] = (uint8_t *)frame->data[0] + size;
            frame->data[2] = (uint8_t *)frame->data[1] + size;
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        frame->linesize[2] = width;
        frame->planes = 3;
        break;
    case PIXEL_FORMAT_RGB3:
    case PIXEL_FORMAT_BGR3:
        size = width * height * 3;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
        }
        frame->linesize[0] = width * 3;
        frame->planes = 1;
        break;
    case PIXEL_FORMAT_I422:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[1] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[2] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
            frame->data[1] = (uint8_t *)frame->data[0] + frame->plane_offsets[1];
            frame->data[2] = (uint8_t *)frame->data[0] + frame->plane_offsets[2];
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        frame->planes = 3;
        break;
    case PIXEL_FORMAT_I40A:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[1] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[2] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[3] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
            frame->data[1] = (uint8_t *)frame->data[0] + frame->plane_offsets[1];
            frame->data[2] = (uint8_t *)frame->data[0] + frame->plane_offsets[2];
            frame->data[3] = (uint8_t *)frame->data[0] + frame->plane_offsets[3];
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        frame->linesize[3] = width;
        frame->planes = 4;
        break;
    case PIXEL_FORMAT_I42A:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[1] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[2] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[3] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
            frame->data[1] = (uint8_t *)frame->data[0] + frame->plane_offsets[1];
            frame->data[2] = (uint8_t *)frame->data[0] + frame->plane_offsets[2];
            frame->data[3] = (uint8_t *)frame->data[0] + frame->plane_offsets[3];
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        frame->linesize[3] = width;
        frame->planes = 4;
        break;
    case PIXEL_FORMAT_YUVA:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[1] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[2] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->plane_offsets[3] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
            frame->data[1] = (uint8_t *)frame->data[0] + frame->plane_offsets[1];
            frame->data[2] = (uint8_t *)frame->data[0] + frame->plane_offsets[2];
            frame->data[3] = (uint8_t *)frame->data[0] + frame->plane_offsets[3];
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        frame->linesize[2] = width;
        frame->linesize[3] = width;
        frame->planes = 4;
        break;
    case PIXEL_FORMAT_JPEG:
    case PIXEL_FORMAT_MJPG:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (mem_type == MEDIA_MEM_DEEP) {
            frame->data[0] = aligned_alloc(ALIGNMENT, size);
        }
        frame->linesize[0] = width;
        frame->planes = 1;
        break;
    default:
        printf("unsupport video format %d\n", format);
        break;
    }
    return 0;
}

void video_frame_deinit(struct video_frame *frame)
{
    if (frame) {
        if (frame->mem_type == MEDIA_MEM_DEEP) {
            aligned_free(frame->data[0]);
        }
    }
}

struct video_frame *video_frame_create(enum pixel_format format,
                uint32_t width, uint32_t height, media_mem_type_t type)
{
    struct video_frame *frame;

    if (format == PIXEL_FORMAT_NONE || width == 0 || height == 0) {
        printf("invalid paramenters!\n");
        return NULL;
    }

    frame = calloc(1, sizeof(struct video_frame));
    if (!frame) {
        printf("malloc video frame failed!\n");
        return NULL;
    }
    if (0 != video_frame_init(frame, format, width, height, type)) {
        printf("video_frame_init failed!\n");
        free(frame);
        frame = NULL;
    }

    return frame;
}

void video_frame_destroy(struct video_frame *frame)
{
    if (frame) {
        video_frame_deinit(frame);
        free(frame);
    }
}

struct video_frame *video_frame_copy(struct video_frame *dst, const struct video_frame *src)
{
    if (!dst || !src) {
        printf("%s invalid paramenters!\n", __func__);
        return NULL;
    }
    switch (src->format) {
    case PIXEL_FORMAT_NONE:
        return NULL;
    case PIXEL_FORMAT_I420:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height / 2);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height / 2);
        break;
    case PIXEL_FORMAT_NV12:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height / 2);
        break;
    case PIXEL_FORMAT_Y800:
    case PIXEL_FORMAT_YVYU:
    case PIXEL_FORMAT_YUY2:
    case PIXEL_FORMAT_UYVY:
    case PIXEL_FORMAT_RGBA:
    case PIXEL_FORMAT_BGRA:
    case PIXEL_FORMAT_BGRX:
    case PIXEL_FORMAT_RGB3:
    case PIXEL_FORMAT_BGR3:
    case PIXEL_FORMAT_AYUV:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        break;
    case PIXEL_FORMAT_I444:
    case PIXEL_FORMAT_I422:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height);
        break;
    case PIXEL_FORMAT_I40A:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height / 2);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height / 2);
        memcpy(dst->data[3], src->data[3], src->linesize[3] * src->height);
        break;
    case PIXEL_FORMAT_I42A:
    case PIXEL_FORMAT_YUVA:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height);
        memcpy(dst->data[3], src->data[3], src->linesize[3] * src->height);
        break;
    default:
        printf("%s invalid pixel format!\n", __func__);
        return NULL;
    }
    dst->timestamp = src->timestamp;
    dst->frame_id = src->frame_id;
    return dst;
}

void video_producer_dump(struct video_producer *vs)
{
    if (!vs) {
        printf("video producer is empty!\n");
        return;
    }
    printf("==== video producer info ====\n");
    printf("format: %s\n", pixel_format_to_string(vs->format));
    printf("resolution: %" PRIu32 "*%" PRIu32 " @ %d/%dfps\n", vs->width, vs->height, vs->framerate.den, vs->framerate.num);
    printf("=============================\n");
}

struct video_packet *video_packet_create(media_mem_type_t type, void *data, size_t len)
{
    struct video_packet *vp = calloc(1, sizeof(struct video_packet));
    if (!vp) {
        return NULL;
    }
    vp->mem_type = type;
    switch (type) {
    case MEDIA_MEM_DEEP:
        vp->data = memdup(data, len);
        vp->size = len;
        break;
    case MEDIA_MEM_SHALLOW:
        vp->data = data;
        vp->size = len;
        break;
    default:
        printf("%s invalid type!\n", __func__);
        break;
    }

    return vp;
}

void video_packet_destroy(struct video_packet *vp)
{
    if (!vp)
        return;
    switch (vp->mem_type) {
    case MEDIA_MEM_DEEP:
        if (vp->data) {
            free(vp->data);
        }
        break;
    case MEDIA_MEM_SHALLOW:
        vp->data = NULL;
        break;
    default:
        printf("%s invalid type!\n", __func__);
        break;
    }
    free(vp);
}

struct video_packet *video_packet_copy(struct video_packet *dst, const struct video_packet *src, media_mem_type_t type)
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
    dst->type = src->type;
    dst->pts  = src->pts;
    dst->dts  = src->dts;
    dst->key_frame = src->key_frame;
    dst->encoder = src->encoder;
    return dst;
}

void video_encoder_dump(struct video_encoder *ve)
{
    if (!ve) {
        printf("video encoder is empty!\n");
        return;
    }
    printf("==== video encoder info ====\n");
    printf("type: %s\n", video_codec_type_to_string(ve->type));
    printf("format: %s\n", pixel_format_to_string(ve->format));
    printf("resolution: %" PRIu32 "*%" PRIu32 " @ %d/%dfps\n", ve->width, ve->height, ve->framerate.den, ve->framerate.num);
    printf("============================\n");
}
