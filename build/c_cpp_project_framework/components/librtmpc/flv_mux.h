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
#ifndef FLV_MUX_H
#define FLV_MUX_H

#include <libmedia-io.h>
#include <libserializer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (flv_mux_output_cb)(void *ctx, uint8_t *data, size_t size, int stream_idx);

struct flv_muxer {
    struct audio_encoder *audio;
    struct video_encoder *video;
    flv_mux_output_cb   *output_cb;
    void                *output_cb_ctx;
    struct serializer    s;
    bool                 is_header;
    bool                 is_keyframe_got;
};

struct flv_muxer *flv_mux_create(flv_mux_output_cb *cb, void *cb_ctx);
int flv_mux_add_media(struct flv_muxer *flv, struct media_packet *pkt);
void flv_mux_destroy(struct flv_muxer *flv);

int flv_write_packet(struct flv_muxer *flv, struct media_packet *pkt);

#ifdef __cplusplus
}
#endif
#endif
