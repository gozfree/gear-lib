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
#include "memalign.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

const char *video_format_name(enum video_format format)
{
    switch (format) {
    case VIDEO_FORMAT_I420:
        return "I420";
    case VIDEO_FORMAT_NV12:
        return "NV12";
    case VIDEO_FORMAT_I422:
        return "I422";
    case VIDEO_FORMAT_YVYU:
        return "YVYU";
    case VIDEO_FORMAT_YUY2:
        return "YUY2";
    case VIDEO_FORMAT_UYVY:
        return "UYVY";
    case VIDEO_FORMAT_RGBA:
        return "RGBA";
    case VIDEO_FORMAT_BGRA:
        return "BGRA";
    case VIDEO_FORMAT_BGRX:
        return "BGRX";
    case VIDEO_FORMAT_I444:
        return "I444";
    case VIDEO_FORMAT_Y800:
        return "Y800";
    case VIDEO_FORMAT_BGR3:
        return "BGR3";
    case VIDEO_FORMAT_I40A:
        return "I40A";
    case VIDEO_FORMAT_I42A:
        return "I42A";
    case VIDEO_FORMAT_YUVA:
        return "YUVA";
    case VIDEO_FORMAT_AYUV:
        return "AYUV";
    case VIDEO_FORMAT_NONE:;
    }

    return "None";
}

#define MAKE_FOURCC(a, b, c, d) \
    ((uint32_t)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

enum video_format video_format_from_fourcc(uint32_t fourcc)
{
    switch (fourcc) {
    case MAKE_FOURCC('Y', '4', '4', '4'):
        return VIDEO_FORMAT_I444;
    case MAKE_FOURCC('U', 'Y', 'V', 'Y'):
    case MAKE_FOURCC('H', 'D', 'Y', 'C'):
    case MAKE_FOURCC('U', 'Y', 'N', 'V'):
    case MAKE_FOURCC('U', 'Y', 'N', 'Y'):
    case MAKE_FOURCC('u', 'y', 'v', '1'):
    case MAKE_FOURCC('2', 'v', 'u', 'y'):
    case MAKE_FOURCC('2', 'V', 'u', 'y'):
        return VIDEO_FORMAT_UYVY;
    case MAKE_FOURCC('Y', 'U', 'Y', 'V'):
    case MAKE_FOURCC('Y', 'U', 'Y', '2'):
    case MAKE_FOURCC('Y', '4', '2', '2'):
    case MAKE_FOURCC('V', '4', '2', '2'):
    case MAKE_FOURCC('V', 'Y', 'U', 'Y'):
    case MAKE_FOURCC('Y', 'U', 'N', 'V'):
    case MAKE_FOURCC('y', 'u', 'v', '2'):
    case MAKE_FOURCC('y', 'u', 'v', 's'):
        return VIDEO_FORMAT_YUY2;
    case MAKE_FOURCC('Y', 'V', 'Y', 'U'):
        return VIDEO_FORMAT_YVYU;
    case MAKE_FOURCC('Y', '8', '0', '0'):
        return VIDEO_FORMAT_Y800;
    }
    return VIDEO_FORMAT_NONE;
}

struct video_frame *video_frame_create(enum video_format format,
                uint32_t width, uint32_t height)
{
    size_t size;
    struct video_frame *frame;
    size_t offsets[VIDEO_MAX_PLANES];

    if (format == VIDEO_FORMAT_NONE || width == 0 || height == 0) {
        printf("invalid paramenters!\n");
        return NULL;
    }

    frame = calloc(1, sizeof(struct video_frame));
    if (!frame) {
        printf("malloc video frame failed!\n");
        return NULL;
    }

    memset(offsets, 0, sizeof(offsets));

    switch (format) {
    case VIDEO_FORMAT_I420:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[0] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[1] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
        frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        break;
    case VIDEO_FORMAT_NV12:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[0] = size;
        size += (width / 2) * (height / 2) * 2;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        break;
    case VIDEO_FORMAT_Y800:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->linesize[0] = width;
        break;
    case VIDEO_FORMAT_YVYU:
    case VIDEO_FORMAT_YUY2:
    case VIDEO_FORMAT_UYVY:
        size = width * height * 2;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->linesize[0] = width * 2;
        break;
    case VIDEO_FORMAT_RGBA:
    case VIDEO_FORMAT_BGRA:
    case VIDEO_FORMAT_BGRX:
    case VIDEO_FORMAT_AYUV:
        size = width * height * 4;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->linesize[0] = width * 4;
        break;
    case VIDEO_FORMAT_I444:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size * 3);
        frame->data[1] = (uint8_t *)frame->data[0] + size;
        frame->data[2] = (uint8_t *)frame->data[1] + size;
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        frame->linesize[2] = width;
        break;
    case VIDEO_FORMAT_BGR3:
        size = width * height * 3;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->linesize[0] = width * 3;
        break;
    case VIDEO_FORMAT_I422:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[0] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[1] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
        frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        break;
    case VIDEO_FORMAT_I40A:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[0] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[1] = size;
        size += (width / 2) * (height / 2);
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[2] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
        frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
        frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        frame->linesize[3] = width;
        break;
    case VIDEO_FORMAT_I42A:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[0] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[1] = size;
        size += (width / 2) * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[2] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
        frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
        frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;
        frame->linesize[3] = width;
        break;
    case VIDEO_FORMAT_YUVA:
        size = width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[0] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[1] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        offsets[2] = size;
        size += width * height;
        size = ALIGN_SIZE(size, ALIGNMENT);
        frame->totoal_size = size;
        frame->data[0] = memalign(ALIGNMENT, size);
        frame->data[1] = (uint8_t *)frame->data[0] + offsets[0];
        frame->data[2] = (uint8_t *)frame->data[0] + offsets[1];
        frame->data[3] = (uint8_t *)frame->data[0] + offsets[2];
        frame->linesize[0] = width;
        frame->linesize[1] = width;
        frame->linesize[2] = width;
        frame->linesize[3] = width;
        break;
    default:
        printf("unsupport video format %d\n", format);
        free(frame);
        frame = NULL;
        break;
    }
    return frame;
}

void video_frame_free(struct video_frame *frame)
{
    if (frame) {
        free(frame->data[0]);
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
    case VIDEO_FORMAT_NONE:
        return NULL;
    case VIDEO_FORMAT_I420:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height / 2);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height / 2);
        break;
    case VIDEO_FORMAT_NV12:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height / 2);
        break;
    case VIDEO_FORMAT_Y800:
    case VIDEO_FORMAT_YVYU:
    case VIDEO_FORMAT_YUY2:
    case VIDEO_FORMAT_UYVY:
    case VIDEO_FORMAT_RGBA:
    case VIDEO_FORMAT_BGRA:
    case VIDEO_FORMAT_BGRX:
    case VIDEO_FORMAT_BGR3:
    case VIDEO_FORMAT_AYUV:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        break;
    case VIDEO_FORMAT_I444:
    case VIDEO_FORMAT_I422:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height);
        break;
    case VIDEO_FORMAT_I40A:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height / 2);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height / 2);
        memcpy(dst->data[3], src->data[3], src->linesize[3] * src->height);
        break;
    case VIDEO_FORMAT_I42A:
    case VIDEO_FORMAT_YUVA:
        memcpy(dst->data[0], src->data[0], src->linesize[0] * src->height);
        memcpy(dst->data[1], src->data[1], src->linesize[1] * src->height);
        memcpy(dst->data[2], src->data[2], src->linesize[2] * src->height);
        memcpy(dst->data[3], src->data[3], src->linesize[3] * src->height);
        break;
    }
    return dst;
}
