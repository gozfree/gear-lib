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

/******************************************************************************
 * uncompressed video define
 ******************************************************************************/

enum pixel_format {
    PIXEL_FORMAT_NONE,

    /* planar 420 format */
    PIXEL_FORMAT_I420, /* three-plane */
    PIXEL_FORMAT_NV12, /* two-plane, luma and packed chroma */

    /* packed 422 formats */
    PIXEL_FORMAT_YVYU,
    PIXEL_FORMAT_YUY2, /* YUYV */
    PIXEL_FORMAT_UYVY,

    /* packed uncompressed formats */
    PIXEL_FORMAT_RGBA,
    PIXEL_FORMAT_BGRA,
    PIXEL_FORMAT_BGRX,
    PIXEL_FORMAT_Y800, /* grayscale */

    /* planar 4:4:4 */
    PIXEL_FORMAT_I444,

    /* more packed uncompressed formats */
    PIXEL_FORMAT_BGR3,

    /* planar 4:2:2 */
    PIXEL_FORMAT_I422,

    /* planar 4:2:0 with alpha */
    PIXEL_FORMAT_I40A,

    /* planar 4:2:2 with alpha */
    PIXEL_FORMAT_I42A,

    /* planar 4:4:4 with alpha */
    PIXEL_FORMAT_YUVA,

    /* packed 4:4:4 with alpha */
    PIXEL_FORMAT_AYUV,

    PIXEL_FORMAT_JPEG,
    PIXEL_FORMAT_MJPG,
};

#ifndef VIDEO_MAX_PLANES
#define VIDEO_MAX_PLANES 8
#endif

struct video_frame {
    uint8_t          *data[VIDEO_MAX_PLANES];
    uint32_t          linesize[VIDEO_MAX_PLANES];
    uint32_t          plane_offsets[VIDEO_MAX_PLANES];
    uint8_t           planes;
    uint64_t          total_size;
    enum pixel_format format;
    uint32_t          width;
    uint32_t          height;
    uint64_t          timestamp;//ns
    uint64_t          frame_id;
    int               flag;
};

const char *pixel_format_name(enum pixel_format format);
enum pixel_format pixel_format_from_fourcc(uint32_t fourcc);

#define VFC_NONE    0   /* nothing to do */
#define VFC_ALLOC   1   /* alloc frame->data */

int video_frame_init(struct video_frame *frame, enum pixel_format format,
                uint32_t width, uint32_t height, int flag);
struct video_frame *video_frame_create(enum pixel_format format,
                uint32_t width, uint32_t height, int flag);
void video_frame_destroy(struct video_frame *frame);
struct video_frame *video_frame_copy(struct video_frame *dst,
                const struct video_frame *src);


/******************************************************************************
 * compressed video define
 ******************************************************************************/
enum video_codec_format {
    VIDEO_CODEC_H264,
    VIDEO_CODEC_AVC = VIDEO_CODEC_H264,
    VIDEO_CODEC_H265,
    VIDEO_CODEC_HEVC = VIDEO_CODEC_H265,
};

enum h264_nal_type {
    H264_NAL_UNKNOWN = 0,
    H264_NAL_SLICE,
    H264_NAL_DPA,
    H264_NAL_DPB,
    H264_NAL_DPC,
    H264_NAL_IDR_SLICE,
    H264_NAL_SEI,
    H264_NAL_SPS,
    H264_NAL_PPS,
    H264_NAL_AUD,
    H264_NAL_END_SEQUENCE,
    H264_NAL_END_STREAM,
    H264_NAL_FILLER_DATA,
    H264_NAL_SPS_EXT,
    H264_NAL_AUXILIARY_SLICE,
};

/**
 * This structure describe encoder attribute
 */
struct video_encoder {
    enum video_codec_format format;
    uint32_t                width;
    uint32_t                height;
    double                  bitrate;
    rational_t              framerate;
    rational_t              timebase;
    uint32_t                start_dts_offset;
    uint32_t                start_dts;
    uint8_t                *extra_data;
    size_t                  extra_size;
};

/**
 * This structure stores compressed data.
 */
struct video_packet {
    uint8_t             *data;
    size_t               size;
    uint64_t             pts;
    uint64_t             dts;
    bool                 key_frame;
    struct video_encoder encoder;
};

struct video_packet *video_packet_create(void *data, size_t len);
void video_packet_destroy(struct video_packet *vp);

#ifdef __cplusplus
}
#endif
#endif
