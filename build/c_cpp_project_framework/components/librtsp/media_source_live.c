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
#include <libavcap.h>
#include <liblog.h>
#include <libdarray.h>
#include <libtime.h>
#include "sdp.h"
#include "media_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <x264.h>

#ifdef __cplusplus
}
#endif

struct x264_ctx {
    enum pixel_format input_format;
    struct iovec sei;
    DARRAY(uint8_t) packet_data;
    x264_param_t param;
    x264_t *handle;
    bool first_frame;
    bool append_extra;
    uint64_t cur_pts;
    uint32_t timebase_num;
    uint32_t timebase_den;
    struct video_encoder encoder;
    void *parent;
};

struct live_source_ctx {
    const char name[32];
    struct avcap_config conf;
    struct avcap_ctx *uvc;
    bool uvc_opened;
    struct x264_ctx *x264;
    struct iovec extradata;
    struct media_frame frm;
    struct media_packet *pkt;
    void *priv;
};

static struct live_source_ctx g_live;

static int pixel_format_to_x264_csp(enum pixel_format fmt)
{
    switch (fmt) {
#if X264_BUILD >= 149
    case PIXEL_FORMAT_UYVY:
        return X264_CSP_UYVY;
#endif
#if X264_BUILD >= 152
    case PIXEL_FORMAT_YUY2:
        return X264_CSP_YUYV;
#endif
    case PIXEL_FORMAT_NV12:
        return X264_CSP_NV12;
    case PIXEL_FORMAT_I420:
        return X264_CSP_I420;
    case PIXEL_FORMAT_I444:
        return X264_CSP_I444;
    case PIXEL_FORMAT_I422:
        return X264_CSP_I422;
    default:
        loge("invalid pixel_format %d\n", fmt);
        return X264_CSP_NONE;
    }
}

static int init_header(struct x264_ctx *c)
{
    x264_nal_t *nals;
    int nal_cnt, nal_bytes, i;

    DARRAY(uint8_t) extra;
    DARRAY(uint8_t) sei;

    da_init(extra);
    da_init(sei);

    nal_bytes = x264_encoder_headers(c->handle, &nals, &nal_cnt);
    if (nal_bytes < 0) {
        loge("x264_encoder_headers failed!\n");
        return -1;
    }

    for (i = 0; i < nal_cnt; i++) {
        x264_nal_t *nal = nals + i;
        if (nal->i_type == NAL_SEI) {
            da_push_back_array(sei, nal->p_payload, nal->i_payload);
        } else {
            da_push_back_array(extra, nal->p_payload, nal->i_payload);
        }
    }

    c->encoder.extra_data = extra.array;
    c->encoder.extra_size = extra.num;
    c->sei.iov_base = sei.array;
    c->sei.iov_len = sei.num;
    //DUMP_BUFFER(c->encoder.extra_data, c->encoder.extra_size);
    logd("encoder.extra_data=%p, encoder.extra_size=%d\n",
          c->encoder.extra_data, c->encoder.extra_size);

    return 0;
}

static struct x264_ctx *x264_open(struct live_source_ctx *cc)
{
    struct x264_ctx *c = calloc(1, sizeof(struct x264_ctx));
    if (!c) {
        loge("malloc x264_ctx failed!\n");
        return NULL;
    }
    x264_param_default_preset(&c->param, "ultrafast" , "zerolatency");
    c->input_format = cc->uvc->conf.video.format;

    c->param.rc.i_vbv_max_bitrate = 2500;
    c->param.rc.i_vbv_buffer_size = 2500;
    c->param.rc.i_bitrate = 2500;
    c->param.rc.i_rc_method = X264_RC_ABR;
    c->param.rc.b_filler = true;
    c->param.i_keyint_max = 1;
    c->param.b_repeat_headers = 1;
    c->param.b_vfr_input = 0;
    c->param.i_log_level = X264_LOG_INFO;
    c->param.i_csp = pixel_format_to_x264_csp(c->input_format);

    c->param.i_width = cc->uvc->conf.video.width;
    c->param.i_height = cc->uvc->conf.video.height;
    c->param.i_fps_num = cc->uvc->conf.video.fps.num;
    c->param.i_fps_den = cc->uvc->conf.video.fps.den;

    x264_param_apply_profile(&c->param, NULL);

    c->handle = x264_encoder_open(&c->param);
    if (c->handle == 0) {
        loge("x264_encoder_open failed!\n");
        goto failed;
    }

    c->first_frame = true;
    c->append_extra = false;

    c->timebase_num = c->param.i_fps_den;
    c->timebase_den = c->param.i_fps_num;

    if (init_header(c)) {
        loge("init_header failed!\n");
        goto failed;
    }

    c->encoder.format = VIDEO_CODEC_H264;
    c->encoder.width = cc->uvc->conf.video.width;
    c->encoder.height = cc->uvc->conf.video.height;
    c->encoder.bitrate = 2500;
    c->encoder.framerate.num = cc->uvc->conf.video.fps.num;
    c->encoder.framerate.den = cc->uvc->conf.video.fps.den;
    c->encoder.timebase.num = c->param.i_fps_den;
    c->encoder.timebase.den = c->param.i_fps_num;
    logi("width=%d, height=%d, timebase=%d/%d\n", c->encoder.width, c->encoder.height, c->encoder.timebase.num, c->encoder.timebase.den);

    c->parent = cc;
    cc->priv = c;
    return c;

failed:
    if (c->handle) {
        x264_encoder_close(c->handle);
        c->handle = 0;
    }
    if (c) {
        free(c);
    }
    return NULL;
}

static int init_pic_data(struct x264_ctx *c, x264_picture_t *pic,
                struct video_frame *frame)
{
    int i;
    x264_picture_init(pic);
    pic->i_pts = frame->timestamp;
    pic->img.i_csp = c->param.i_csp;

    switch (c->param.i_csp) {
#if X264_BUILD >= 149
    case X264_CSP_YUYV:
        pic->img.i_plane = 1;
        break;
#endif
    case X264_CSP_NV12:
        pic->img.i_plane = 2;
        break;
    case X264_CSP_I420:
    case X264_CSP_I444:
    case X264_CSP_I422:
        pic->img.i_plane = 3;
        break;
    default:
        loge("unsupport colorspace type %d\n", c->param.i_csp);
        break;
    }
    if (pic->img.i_plane != frame->planes) {
        loge("video frame planes mismatch: pic->img.i_plane=%d, frame->planes=%d\n",
pic->img.i_plane, frame->planes);
        return -1;
    }

    for (i = 0; i < pic->img.i_plane; i++) {
        pic->img.i_stride[i] = (int)frame->linesize[i];
        pic->img.plane[i] = frame->data[i];
    }
    return 0;
}

static int fill_packet(struct x264_ctx *c, struct video_packet *pkt,
                       x264_nal_t *nals, int nal_cnt, x264_picture_t *pic_out)
{
    int i;
    if (!nal_cnt)
        return -1;

    da_free(c->packet_data);

    if (!c->append_extra) {
        pkt->encoder.extra_data = c->encoder.extra_data;
        pkt->encoder.extra_size = c->encoder.extra_size;
        c->append_extra = true;
    }
    for (i = 0; i < nal_cnt; i++) {
        x264_nal_t *nal = nals + i;
        da_push_back_array(c->packet_data, nal->p_payload, nal->i_payload);
    }

    pkt->data = c->packet_data.array;
    pkt->size = c->packet_data.num;
    pkt->pts = pic_out->i_pts;
    pkt->dts = pic_out->i_dts;
    pkt->encoder.timebase.num = c->param.i_fps_den;
    pkt->encoder.timebase.den = c->param.i_fps_num;
    logd("pkt->dts=%d, pkt->encoder.timebase = %d/%d\n",
          pkt->dts, pkt->encoder.timebase.num, pkt->encoder.timebase.den);

    pkt->key_frame = pic_out->b_keyframe != 0;
    pkt->type = pkt->key_frame ? H26X_FRAME_I : H26X_FRAME_P;

    memcpy(&pkt->encoder, &c->encoder, sizeof(struct video_encoder));
    logd("pkt->encoder.extra_size = %d\n", pkt->encoder.extra_size);
    return pkt->size;
}

static int x264_encode(struct x264_ctx *c, struct iovec *in, struct iovec *out)
{
    x264_picture_t pic_in, pic_out;
    x264_nal_t *nal;
    int nal_cnt = 0;
    int ret = 0;
    int nal_bytes = 0;
    struct video_frame *frm = in->iov_base;
    struct media_packet *mpkt = out->iov_base;
    struct video_packet *pkt = mpkt->video;
    if (c->first_frame) {
        c->cur_pts = 0;
        c->first_frame = false;
    }
    frm->timestamp = c->cur_pts;

    init_pic_data(c, &pic_in, frm);

    nal_bytes = x264_encoder_encode(c->handle, &nal, &nal_cnt, &pic_in,
                    &pic_out);
    if (nal_bytes < 0) {
        loge("x264_encoder_encode failed!\n");
        return -1;
    }
    ret = fill_packet(c, pkt, nal, nal_cnt, &pic_out);
    if (ret < 0) {
        loge("fill_packet failed!\n");
        return -1;
    }

    logd("frame info: <id=%d, pts=%zu>; packet info: <pts=%zu, dts=%zu, keyframe=%d, size=%zu>\n",
        frm->frame_id, frm->timestamp, pkt->pts, pkt->dts, pkt->key_frame, pkt->size);
    c->cur_pts += c->timebase_num;
    out->iov_len = ret;
    logd("encode size=%d\n", out->iov_len);
    return ret;
}

static void x264_close(struct x264_ctx *c)
{
    struct live_source_ctx *cc = (struct live_source_ctx *)c->parent;
    if (c->handle) {
        //x264_encoder_close(c->handle);//XXX cause segfault

        free(cc->extradata.iov_base);
        cc->extradata.iov_len = 0;
    }
    free(c->sei.iov_base);
    free(c);
}
static int is_auth()
{
    return 0;
}

static uint32_t get_random_number()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    srand(now.tv_usec);
    return (rand() % ((uint32_t)-1));
}

static int live_open(struct media_source *ms, const char *name)
{
    struct live_source_ctx *c = &g_live;
    g_live.uvc_opened = false;
    if (c->uvc_opened) {
        logi("uvc already opened!\n");
        return 0;
    }
    c->conf.video.width = 640;
    c->conf.video.height = 480;
    c->conf.video.fps.num = 30;
    c->conf.video.fps.den = 1;
    c->conf.video.format = PIXEL_FORMAT_YUY2,
    c->uvc = avcap_open("/dev/video0", &c->conf);
    if (!c->uvc) {
        loge("uvc open failed!\n");
        return -1;
    }
    video_frame_init(&c->frm.video, c->uvc->conf.video.format, c->uvc->conf.video.width, c->uvc->conf.video.height, MEDIA_MEM_SHALLOW);
    c->pkt = media_packet_create(MEDIA_TYPE_VIDEO, MEDIA_MEM_SHALLOW, NULL, 0);
    if (avcap_start_stream(c->uvc, NULL)) {
        loge("uvc start stream failed!\n");
        return -1;
    }
    c->uvc_opened = true;
    ms->opaque = c;
    c->x264 = x264_open(c);
    if (!c->x264) {
        loge("x264_open failed!\n");
        return -1;
    }
    return 0;
}

static void live_close(struct media_source *ms)
{
    struct live_source_ctx *c = (struct live_source_ctx *)ms->opaque;
    avcap_stop_stream(c->uvc);
    x264_close(c->x264);
    avcap_close(c->uvc);
    c->uvc_opened = false;
}

static int sdp_generate(struct media_source *ms)
{
    int n = 0;
    char p[SDP_LEN_MAX];
    uint32_t session_id = get_random_number();
    gettimeofday(&ms->tm_create, NULL);
    n += snprintf(p+n, sizeof(p)-n, "v=0\n");
    n += snprintf(p+n, sizeof(p)-n, "o=%s %"PRIu32" %"PRIu32" IN IP4 %s\n", is_auth()?"username":"-", session_id, 1, "0.0.0.0");
    n += snprintf(p+n, sizeof(p)-n, "s=%s\n", ms->name);
    n += snprintf(p+n, sizeof(p)-n, "i=%s\n", ms->info);
    n += snprintf(p+n, sizeof(p)-n, "c=IN IP4 0.0.0.0\n");
    n += snprintf(p+n, sizeof(p)-n, "t=0 0\n");
    n += snprintf(p+n, sizeof(p)-n, "a=range:npt=0-\n");
    n += snprintf(p+n, sizeof(p)-n, "a=sendonly\n");
    n += snprintf(p+n, sizeof(p)-n, "a=control:*\n");
    n += snprintf(p+n, sizeof(p)-n, "a=source-filter: incl IN IP4 * %s\r\n", "0.0.0.0");
    n += snprintf(p+n, sizeof(p)-n, "a=rtcp-unicast: reflection\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=x-qt-text-nam:%s\r\n", ms->name);
    n += snprintf(p+n, sizeof(p)-n, "a=x-qt-text-inf:%s\r\n", ms->info);

    n += snprintf(p+n, sizeof(p)-n, "m=video 0 RTP/AVP 96\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=rtpmap:96 H264/90000\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=fmtp:96 packetization-mode=1; profile-level-id=4D4028; sprop-parameter-sets=Z01AKJpkA8ARPy4C3AQEBQAAAwPoAADqYOhgBGMAAF9eC7y40MAIxgAAvrwXeXCg,aO44gA==;\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=cliprect:0,0,240,320\r\n");
    strcpy(ms->sdp, p);
    return 0;
}

static int live_read(struct media_source *ms, void **data, size_t *len)
{
    int ret;
    uint64_t ms_pre, ms_post;
    struct iovec in, out;
    struct live_source_ctx *c = (struct live_source_ctx *)ms->opaque;
    int size;
    ms_pre = time_now_msec();
    size = avcap_query_frame(c->uvc, &c->frm);
    if (size < 0) {
        loge("avcap_query_frame failed!\n");
    }
    ms_post = time_now_msec();
    logd("avcap_query_frame cost %" PRIu64 "ms\n", ms_post - ms_pre);
    in.iov_base = &c->frm.video;
    in.iov_len = size;
    out.iov_base = c->pkt;
    ms_pre = time_now_msec();
    ret = x264_encode(c->x264, &in, &out);
    if (ret < 0) {
        loge("x264_encode failed\n");
    }
    ms_post = time_now_msec();
    *data = out.iov_base;
    *len = out.iov_len;
    logd("x264_encode len=%d, cost %" PRIu64 "ms\n", *len, ms_post - ms_pre);
    return 0;
}

struct media_source media_source_uvc = {
    .name         = "uvc",
    .sdp_generate = sdp_generate,
    ._open         = live_open,
    ._read         = live_read,
    ._close        = live_close,
};
