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
#include "libmp4.h"
#include <stdio.h>
#include <stdlib.h>

static int muxer_add_stream(struct mp4_muxer *muxer, struct mp4_muxer_media *media, enum AVCodecID codec_id)
{
    AVCodec *codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        printf("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return -1;
    }
    media->first_pts = 0;
    media->last_pts = 0;

    media->av_stream = avformat_new_stream(muxer->av_format, codec);
    if (!media->av_stream) {
        printf("Could not allocate stream\n");
        return -1;
    }
    media->av_stream->id = muxer->av_format->nb_streams-1;
    //AVCodecContext *c = media->av_stream->codec;
    AVCodecParameters *cp = media->av_stream->codecpar;

    switch (codec->type) {
    case AVMEDIA_TYPE_AUDIO:
        cp->format  = AV_SAMPLE_FMT_FLTP;
        cp->bit_rate    = 32000;
        cp->sample_rate = 8000;
        cp->channel_layout = AV_CH_LAYOUT_STEREO;
        cp->channels       = av_get_channel_layout_nb_channels(cp->channel_layout);
        cp->frame_size  = 1000;
        media->av_stream->time_base = (AVRational){ 1, cp->sample_rate };
        muxer->audio.av_bsf = av_bsf_get_by_name("aac_adtstoasc");
        break;
    case AVMEDIA_TYPE_VIDEO:
        codec_id = AV_CODEC_ID_H264;
        if (codec_id != AV_CODEC_ID_H264 && codec_id != AV_CODEC_ID_H265 &&
            codec_id != AV_CODEC_ID_HEVC && codec_id != AV_CODEC_ID_MJPEG) {
            printf("video codec_id %s not support\n", avcodec_get_name(codec_id));
            return -1;
        }
        cp->codec_id       = codec_id;
        cp->bit_rate       = 2000000;
        cp->width          = muxer->conf.width;
        cp->height         = muxer->conf.height;
        if (muxer->conf.fps.den == 0) {
            printf("warnning: muxer->conf.fps.den must not be 0\n");
            muxer->conf.fps.den = 1;
        }
        media->av_stream->time_base = (AVRational){1, muxer->conf.fps.num/muxer->conf.fps.den};
        cp->format        = AV_PIX_FMT_NV12;
        break;
    default:
        printf("codec type not support\n");
        break;
    }

    return 0;
}

struct mp4_muxer *mp4_muxer_open(const char *file, struct mp4_config *conf)
{
    AVOutputFormat *ofmt = NULL;
    struct mp4_muxer *c = calloc(1, sizeof(struct mp4_muxer));
    if (!c) {
        printf("malloc mp4_muxer failed!\n");
        return NULL;
    }

    av_register_all();
    av_log_set_level(AV_LOG_ERROR);
    avformat_alloc_output_context2(&c->av_format, NULL, "mp4", NULL);
    if (c->av_format == NULL) {
        printf("avformat_alloc_output_context2 failed\n");
        goto failed;
    }

    ofmt = c->av_format->oformat;
    if (ofmt->video_codec != AV_CODEC_ID_NONE) {
        muxer_add_stream(c, &c->video, ofmt->video_codec);
    }
    if (ofmt->audio_codec != AV_CODEC_ID_NONE) {
        muxer_add_stream(c, &c->audio, ofmt->audio_codec);
    }

    if (ofmt->flags & AVFMT_NOFILE) {
        printf("av_format nofile error\n");
        goto failed;
    }
    if (0 > avio_open(&c->av_format->pb, file, AVIO_FLAG_WRITE)) {
        printf("Could not open '%s'\n", file);
        goto failed;
    }

    if (0 > avformat_write_header(c->av_format, NULL)) {
        printf("write header error\n");
        avio_closep(&c->av_format->pb);
        goto failed;
    }
    c->got_video = false;
    memcpy(&c->conf, conf, sizeof(struct mp4_config));
    return c;
failed:
    if (c) {
        free(c);
    }
    return NULL;
}

#if 0
static int64_t update_video_timestamp(struct mp4_muxer_ctx *c, uint64_t pts)
{
    if (c->video.first_pts == 0) {
        c->video.first_pts = pts;
    }
    if (pts < c->video.last_pts) {
        c->video.last_pts = c->video.last_pts + 1;
    } else {
        c->video.last_pts = pts - c->video.first_pts;
    }
    return av_rescale_q(c->video.last_pts, (AVRational){1, 1000}, (AVRational){1, 12500});//fps=25/2
}

int64_t update_audio_timestamp(struct mp4_muxer_ctx *c, uint64_t pts)
{
    if (c->audio.first_pts == 0) {
        c->audio.first_pts = pts;
    }
    if (pts < c->audio.last_pts) {
        c->audio.last_pts = c->audio.last_pts + 1;
    } else {
        c->audio.last_pts = pts - c->audio.first_pts;
    }
    return av_rescale_q(c->audio.last_pts, (AVRational){1, 1000}, c->audio.av_stream->time_base);
}
#endif

int mp4_muxer_write(struct mp4_muxer *c, struct media_packet *mp)
{
    AVPacket pkt;
    av_init_packet(&pkt);

    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        if (c->got_video == false) {
            goto exit;
        }
        //pkt.stream_index = c->audio.av_stream->index;
        pkt.data = mp->audio->data;
        pkt.size = mp->audio->size;
        //pkt.pos = -1;
        //pkt.pts = update_audio_timestamp(c, mp->pts);
        //av_bitstream_filter_filter(c->audio.av_bsf, c->audio.av_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
        break;
    case MEDIA_TYPE_VIDEO:
        if (mp->video->type == H26X_FRAME_I) {
            c->got_video = true;
            pkt.flags |= AV_PKT_FLAG_KEY;
        }
        if (c->got_video == false) {
            goto exit;
        }
        //pkt.stream_index = c->video.av_stream->index;
        pkt.data = mp->video->data;
        pkt.size = mp->video->size;
        //pkt.pos = -1;
        //pkt.pts = update_video_timestamp(c, mp->pts);
        break;
    default:
        printf("unknown mp type\n");
        break;
    }

    if (c->got_video) {
        av_interleaved_write_frame(c->av_format, &pkt);
    }
exit:
    av_packet_unref(&pkt);
    return 0;
}

void mp4_muxer_close(struct mp4_muxer *c)
{
    if (!c)
        return;

    if (0 > av_write_trailer(c->av_format)) {
        printf("av_write_trailer failed!\n");
    }
    if (!(c->av_format->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&c->av_format->pb);
    }
    avformat_free_context(c->av_format);
    free(c);
}
