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
#ifndef LIBMP4_H
#define LIBMP4_H

#include <libposix.h>
#include <libmedia-io.h>
#include <stdlib.h>

#define LIBMP4_VERSION "0.1.0"

#ifdef __cplusplus
extern "C" {
#endif


#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>


struct mp4_muxer_media {
    enum AVCodecID codec_id;
    AVStream *av_stream;
    AVCodec *av_codec;
    const AVBitStreamFilter *av_bsf;
    uint64_t first_pts;
    uint64_t last_pts;
};

struct mp4_config {
    uint32_t width;
    uint32_t height;
    rational_t fps;
    enum pixel_format format;
};

struct mp4_muxer {
    struct mp4_muxer_media audio;
    struct mp4_muxer_media video;
    struct mp4_config conf;
    AVFormatContext *av_format;
    bool got_video;
};

GEAR_API struct mp4_muxer *mp4_muxer_open(const char *file, struct mp4_config *conf);
GEAR_API int mp4_muxer_write(struct mp4_muxer *c, struct media_packet *frame);
GEAR_API void mp4_muxer_close(struct mp4_muxer *c);


struct mp4_parser {
    void *opaque_stream;
    void *opaque_root;
};


GEAR_API struct mp4_parser *mp4_parser_create(const char *file);
GEAR_API int mp4_get_duration(struct mp4_parser *mp, uint64_t *duration);
GEAR_API int mp4_get_creation(struct mp4_parser *mp, uint64_t *time);
GEAR_API int mp4_get_resolution(struct mp4_parser *mp, uint32_t *width, uint32_t *height);
GEAR_API void mp4_parser_destroy(struct mp4_parser *mp);


#ifdef __cplusplus
}
#endif
#endif
