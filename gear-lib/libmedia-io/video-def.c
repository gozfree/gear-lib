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

#define ALIGNMENT 32
#define ALIGN_SIZE(size, align) (((size) + (align - 1)) & (~(align - 1)))

#ifndef FREERTOS
#define aligned_free    free
#endif

#if defined _ISOC11_SOURCE || __USE_ISOC11 || defined __USE_ISOCXX11
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
#endif
const char *pixel_format_name(enum pixel_format format)
{
    switch (format) {
    case PIXEL_FORMAT_I420:
        return "I420";
    case PIXEL_FORMAT_NV12:
        return "NV12";
    case PIXEL_FORMAT_I422:
        return "I422";
    case PIXEL_FORMAT_YVYU:
        return "YVYU";
    case PIXEL_FORMAT_YUY2:
        return "YUY2";
    case PIXEL_FORMAT_UYVY:
        return "UYVY";
    case PIXEL_FORMAT_RGBA:
        return "RGBA";
    case PIXEL_FORMAT_BGRA:
        return "BGRA";
    case PIXEL_FORMAT_BGRX:
        return "BGRX";
    case PIXEL_FORMAT_I444:
        return "I444";
    case PIXEL_FORMAT_Y800:
        return "Y800";
    case PIXEL_FORMAT_BGR3:
        return "BGR3";
    case PIXEL_FORMAT_I40A:
        return "I40A";
    case PIXEL_FORMAT_I42A:
        return "I42A";
    case PIXEL_FORMAT_YUVA:
        return "YUVA";
    case PIXEL_FORMAT_AYUV:
        return "AYUV";
    case PIXEL_FORMAT_JPEG:
        return "JPEG";
    case PIXEL_FORMAT_MJPG:
        return "MJPG";
    default:
        return "PIXEL_FORMAT_UNKNOWN";
    }
    return "PIXEL_FORMAT_UNKNOWN";
}

#define MAKE_FOURCC(a, b, c, d) \
    ((uint32_t)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))


enum pixel_format pixel_format_from_fourcc(uint32_t fourcc)
{
    switch (fourcc) {
    case MAKE_FOURCC('Y', '4', '4', '4'):
        return PIXEL_FORMAT_I444;
    case MAKE_FOURCC('U', 'Y', 'V', 'Y'):
    case MAKE_FOURCC('H', 'D', 'Y', 'C'):
    case MAKE_FOURCC('U', 'Y', 'N', 'V'):
    case MAKE_FOURCC('U', 'Y', 'N', 'Y'):
    case MAKE_FOURCC('u', 'y', 'v', '1'):
    case MAKE_FOURCC('2', 'v', 'u', 'y'):
    case MAKE_FOURCC('2', 'V', 'u', 'y'):
        return PIXEL_FORMAT_UYVY;
    case MAKE_FOURCC('Y', 'U', 'Y', 'V'):
    case MAKE_FOURCC('Y', 'U', 'Y', '2'):
    case MAKE_FOURCC('Y', '4', '2', '2'):
    case MAKE_FOURCC('V', '4', '2', '2'):
    case MAKE_FOURCC('V', 'Y', 'U', 'Y'):
    case MAKE_FOURCC('Y', 'U', 'N', 'V'):
    case MAKE_FOURCC('y', 'u', 'v', '2'):
    case MAKE_FOURCC('y', 'u', 'v', 's'):
        return PIXEL_FORMAT_YUY2;
    case MAKE_FOURCC('Y', 'V', 'Y', 'U'):
        return PIXEL_FORMAT_YVYU;
    case MAKE_FOURCC('Y', 'V', '1', '2'):
        return PIXEL_FORMAT_I420;
    case MAKE_FOURCC('Y', 'U', '1', '2'):
        return PIXEL_FORMAT_I420;
    case MAKE_FOURCC('N', 'V', '1', '2'):
        return PIXEL_FORMAT_NV12;
    case MAKE_FOURCC('X', 'R', '2', '4'):
        return PIXEL_FORMAT_BGRX;
    case MAKE_FOURCC('B', 'G', 'R', '3'):
        return PIXEL_FORMAT_BGR3;
    case MAKE_FOURCC('A', 'R', '2', '4'):
        return PIXEL_FORMAT_BGRA;
    case MAKE_FOURCC('Y', '8', '0', '0'):
        return PIXEL_FORMAT_Y800;
    case MAKE_FOURCC('J', 'P', 'E', 'G'):
        return PIXEL_FORMAT_JPEG;
    case MAKE_FOURCC('M', 'J', 'P', 'G'):
        return PIXEL_FORMAT_MJPG;
    }
    printf("pixel_format_from_fourcc failed!\n");
    return PIXEL_FORMAT_NONE;
}

int video_frame_init(struct video_frame *frame, enum pixel_format format,
                uint32_t width, uint32_t height, int flag)
{
    size_t size;

    if (format == PIXEL_FORMAT_NONE || width == 0 || height == 0) {
        printf("invalid paramenters!\n");
        return -1;
    }

    memset(frame, 0, sizeof(struct video_frame));
    frame->format = format;
    frame->width = width;
    frame->height = height;
    frame->flag = flag;

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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
        frame->data[0] = aligned_alloc(ALIGNMENT, size);
        }
        frame->linesize[0] = width * 4;
        frame->planes = 1;
        break;
    case PIXEL_FORMAT_I444:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (flag == VFC_ALLOC) {
        frame->data[0] = aligned_alloc(ALIGNMENT, size * 3);
        frame->data[1] = (uint8_t *)frame->data[0] + size;
        frame->data[2] = (uint8_t *)frame->data[1] + size;
        }
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        frame->linesize[2] = width;
        frame->planes = 3;
        break;
    case PIXEL_FORMAT_BGR3:
        size = width * height * 3;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->total_size = size;
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
        if (flag == VFC_ALLOC) {
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
    default:
        printf("unsupport video format %d\n", format);
        break;
    }
    return 0;
}

struct video_frame *video_frame_create(enum pixel_format format,
                uint32_t width, uint32_t height, int flag)
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
    if (0 != video_frame_init(frame, format, width, height, flag)) {
        printf("video_frame_init failed!\n");
        free(frame);
        frame = NULL;
    }

    return frame;
}

void video_frame_destroy(struct video_frame *frame)
{
    if (frame) {
        if (frame->flag == VFC_ALLOC) {
            aligned_free(frame->data[0]);
        }
        free(frame);
    }
}

struct video_frame *video_frame_copy(struct video_frame *dst, const struct video_frame *src)
{
    if (!dst || !src) {
        printf("invalid paramenters!\n");
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
        return NULL;
    }
    dst->timestamp = src->timestamp;
    dst->frame_id = src->frame_id;
    return dst;
}

struct video_packet *video_packet_create(void *data, size_t len)
{
    struct video_packet *vp = calloc(1, sizeof(struct video_packet));
    if (!vp) {
        return NULL;
    }
    vp->data = data;
    vp->size = len;
    return vp;
}

void video_packet_destroy(struct video_packet *vp)
{
    if (vp) {
        if (vp->data) {
            free(vp->data);
        }
        free(vp);
    }
}
