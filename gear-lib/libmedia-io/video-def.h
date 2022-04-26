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
    /* refer to v4l2 and ffmpeg */
    PIXEL_FORMAT_NONE,
    /* planar 420 format */
    PIXEL_FORMAT_I420, /* three-plane */
    PIXEL_FORMAT_NV12, /* two-plane, luma and packed chroma */

    /* packed 422 formats */
    PIXEL_FORMAT_YVYU,
    PIXEL_FORMAT_YUY2, /* YUYV */
    PIXEL_FORMAT_UYVY,

    /* packed uncompressed formats */
    PIXEL_FORMAT_0RGB, /* packed RGB 8:8:8, 32bpp, XRGB... X=unused */
    PIXEL_FORMAT_RGBA,

    PIXEL_FORMAT_RGB565BE, /* packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian */
    PIXEL_FORMAT_RGB565LE, /* packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian */
    PIXEL_FORMAT_RGB555BE,  ///< packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), big-endian   , X=unused/undefined
    PIXEL_FORMAT_RGB8,      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    PIXEL_FORMAT_RGB24, /* packed RGB 8:8:8, 24bpp, RGBRGB... */
    PIXEL_FORMAT_BGRA,
    PIXEL_FORMAT_BGRX,
    PIXEL_FORMAT_Y800, /* grayscale */

    /* planar 4:4:4 */
    PIXEL_FORMAT_I444,

    /* 24-bit more packed uncompressed formats */
    PIXEL_FORMAT_RGB3,
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
    PIXEL_FORMAT_MAX,
};

#ifndef VIDEO_MAX_PLANES
#define VIDEO_MAX_PLANES 8
#endif

/**
 * This structure describe attribute of video producer, which is creator
 */
struct video_producer {
    enum pixel_format format;
    uint32_t          width;
    uint32_t          height;
    rational_t        framerate;
};

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
    media_mem_type_t  mem_type;
};

const char *pixel_format_to_string(enum pixel_format fmt);
enum pixel_format pixel_string_to_format(const char *name);

GEAR_API int video_frame_init(struct video_frame *frame, enum pixel_format format,
                uint32_t width, uint32_t height, media_mem_type_t type);

GEAR_API void video_frame_deinit(struct video_frame *frame);
GEAR_API struct video_frame *video_frame_create(enum pixel_format format,
                uint32_t width, uint32_t height, media_mem_type_t type);
GEAR_API void video_frame_destroy(struct video_frame *frame);
GEAR_API struct video_frame *video_frame_copy(struct video_frame *dst,
                const struct video_frame *src);

void video_producer_dump(struct video_producer *vp);

/******************************************************************************
 * compressed video define
 ******************************************************************************/
enum video_codec_type {
    VIDEO_CODEC_NONE,
    VIDEO_CODEC_H264,
    VIDEO_CODEC_AVC = VIDEO_CODEC_H264,
    VIDEO_CODEC_H265,
    VIDEO_CODEC_HEVC = VIDEO_CODEC_H265,
    VIDEO_CODEC_MAX,
};

const char *video_codec_type_to_string(enum video_codec_type type);
enum video_codec_type video_codec_string_to_type(const char *name);

enum video_packet_type {
    H26X_FRAME_UNKNOWN = 0,
    H26X_FRAME_IDR,
    H26X_FRAME_I,
    H26X_FRAME_P,
    H26X_FRAME_B,
    H26X_FRAME_TYPE_MAX,
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
    enum video_codec_type   type;
    enum pixel_format       format;
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
    uint8_t               *data;
    size_t                 size;
    enum video_packet_type type;
    media_mem_type_t       mem_type;
    uint64_t               pts;
    uint64_t               dts;
    bool                   key_frame;
    struct video_encoder   encoder;
};

GEAR_API struct video_packet *video_packet_create(media_mem_type_t type, void *data, size_t len);
GEAR_API void video_packet_destroy(struct video_packet *vp);
GEAR_API struct video_packet *video_packet_copy(struct video_packet *dst, const struct video_packet *src, media_mem_type_t type);
GEAR_API void video_encoder_dump(struct video_encoder *ve);

#ifdef __cplusplus
}
#endif
#endif
