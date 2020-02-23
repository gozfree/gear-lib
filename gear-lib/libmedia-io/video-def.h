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
#ifndef VIDEO_DEF_H
#define VIDEO_DEF_H

#include <stdint.h>

/**
 * This file reference to ffmpeg and obs define
 */

#ifdef __cplusplus
extern "C" {
#endif

#define VIDEO_MAX_PLANES               8

enum video_format {
    VIDEO_FORMAT_NONE,

    /* planar 420 format */
    VIDEO_FORMAT_I420, /* three-plane */
    VIDEO_FORMAT_NV12, /* two-plane, luma and packed chroma */

    /* packed 422 formats */
    VIDEO_FORMAT_YVYU,
    VIDEO_FORMAT_YUY2, /* YUYV */
    VIDEO_FORMAT_UYVY,

    /* packed uncompressed formats */
    VIDEO_FORMAT_RGBA,
    VIDEO_FORMAT_BGRA,
    VIDEO_FORMAT_BGRX,
    VIDEO_FORMAT_Y800, /* grayscale */

    /* planar 4:4:4 */
    VIDEO_FORMAT_I444,

    /* more packed uncompressed formats */
    VIDEO_FORMAT_BGR3,

    /* planar 4:2:2 */
    VIDEO_FORMAT_I422,

    /* planar 4:2:0 with alpha */
    VIDEO_FORMAT_I40A,

    /* planar 4:2:2 with alpha */
    VIDEO_FORMAT_I42A,

    /* planar 4:4:4 with alpha */
    VIDEO_FORMAT_YUVA,

    /* packed 4:4:4 with alpha */
    VIDEO_FORMAT_AYUV,

    VIDEO_FORMAT_JPEG,
    VIDEO_FORMAT_MJPG,
};

const char *video_format_name(enum video_format format);
enum video_format video_format_from_fourcc(uint32_t fourcc);

/**
 * This structure describes decoded (raw) video data.
 */
#ifndef VIDEO_MAX_PLANES
#define VIDEO_MAX_PLANES 8
#endif
struct video_frame {
    uint8_t           *data[VIDEO_MAX_PLANES];
    uint32_t          linesize[VIDEO_MAX_PLANES];
    uint32_t          plane_offsets[VIDEO_MAX_PLANES];
    enum video_format format;
    uint32_t          width;
    uint32_t          height;
    uint64_t          timestamp;//ns
    uint8_t           planes;
    uint64_t          total_size;
    uint64_t          id;
    int               flag;
    uint8_t           **extended_data;
};

#define VFC_NONE    0   /* nothing to do */
#define VFC_ALLOC   1   /* alloc frame->data */

int video_frame_init(struct video_frame *frame, enum video_format format,
                uint32_t width, uint32_t height, int flag);
struct video_frame *video_frame_create(enum video_format format,
                uint32_t width, uint32_t height, int flag);
void video_frame_destroy(struct video_frame *frame);

struct video_frame *video_frame_copy(struct video_frame *dst,
                const struct video_frame *src);

enum video_encode_format {
    VIDEO_ENCODE_H264,
    VIDEO_ENCODE_H265,
};

/**
 * This structure stores compressed data.
 */
struct video_packet {
    uint8_t                  *data;
    size_t                   size;
    enum video_encode_format format;
    uint64_t                 pts;
    uint64_t                 dts;
    uint32_t                 timebase_num;
    uint32_t                 timebase_den;
    int                      key_frame;
    uint8_t                  *extra_data;
    size_t                   extra_size;
};

struct video_packet *video_packet_create(void *data, size_t len);
void video_packet_destroy(struct video_packet *packet);

#ifdef __cplusplus
}
#endif
#endif
