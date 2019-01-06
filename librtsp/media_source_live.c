#include <libuvc.h>
#include <liblog.h>
#include <libmacro.h>
#include "sdp.h"
#include "media_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <x264.h>

#ifdef __cplusplus
}
#endif

struct x264_ctx {
    int width;
    int height;
    int encode_format;
    int input_format;
    struct iovec sei;
    struct iovec pkt;
    x264_param_t *param;
    x264_t *handle;
    x264_picture_t *picture;
    x264_nal_t *nal;
    void *parent;
};

struct live_source_ctx {
    const char name[32];
    struct iovec data;
    struct uvc_ctx *uvc;
    bool uvc_opened;
    int width;
    int height;
    struct x264_ctx *x264;
    struct iovec extradata;
};

static struct live_source_ctx g_live = {.uvc_opened = false};

#define AV_INPUT_BUFFER_PADDING_SIZE 32

static struct x264_ctx *x264_open(struct live_source_ctx *cc)
{
    int m_frameRate = 25;
    //int m_bitRate = 1000;
    struct x264_ctx *c = CALLOC(1, struct x264_ctx);
    if (!c) {
        loge("malloc x264_ctx failed!\n");
        return NULL;
    }
    c->param = CALLOC(1, x264_param_t);
    if (!c->param) {
        loge("malloc param failed!\n");
        goto failed;
    }
    c->picture = CALLOC(1, x264_picture_t);
    if (!c->picture) {
        loge("malloc picture failed!\n");
        goto failed;
    }
    x264_param_default_preset(c->param, "ultrafast" , "zerolatency");

    c->param->i_width = cc->width;
    c->param->i_height = cc->height;
    //XXX b_repeat_headers 0: rtmp is ok; 1: playback is ok
    c->param->b_repeat_headers = 0;  // repeat SPS/PPS before i frame
    c->param->b_vfr_input = 0;
    c->param->b_annexb = 1;
    c->param->b_cabac = 1;
    c->param->i_threads = 1;
    c->param->i_fps_num = (int)m_frameRate;
    c->param->i_fps_den = 1;
    c->param->i_keyint_max = 25;
    c->param->i_log_level = X264_LOG_ERROR;
    x264_param_apply_profile(c->param, "high422");
    c->handle = x264_encoder_open(c->param);
    if (c->handle == 0) {
        loge("x264_encoder_open failed!\n");
        goto failed;
    }

    if (-1 == x264_picture_alloc(c->picture, X264_CSP_I420, c->param->i_width,
            c->param->i_height)) {
        loge("x264_picture_alloc failed!\n");
        goto failed;
    }
    c->picture->img.i_csp = X264_CSP_I420;
    c->picture->img.i_plane = 3;
    c->width = cc->width;
    c->height = cc->height;
    //c->input_format = YUV422P;
    c->pkt.iov_len = 0;
    c->pkt.iov_base = NULL;

    if (1) {//flags & AV_CODEC_FLAG_GLOBAL_HEADER) {
        x264_nal_t *nal;
        uint8_t *p;
        int nnal, s, i;

        s = x264_encoder_headers(c->handle, &nal, &nnal);
        cc->extradata.iov_base = p = (uint8_t *)calloc(1, s + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!p)
            return NULL;

        for (i = 0; i < nnal; i++) {
            /* Don't put the SEI in extradata. */
            if (nal[i].i_type == NAL_SEI) {
                logd("%s\n", nal[i].p_payload+25);
                c->sei.iov_len = nal[i].i_payload;
                c->sei.iov_base = calloc(1, c->sei.iov_len);
                if (!c->sei.iov_base)
                    return NULL;
                memcpy(c->sei.iov_base, nal[i].p_payload, nal[i].i_payload);
                continue;
            }
            memcpy(p, nal[i].p_payload, nal[i].i_payload);
            p += nal[i].i_payload;
        }
        cc->extradata.iov_len = p - (uint8_t *)cc->extradata.iov_base;
    }
    c->parent = cc;
    return c;

failed:
    if (c->handle) {
        x264_encoder_close(c->handle);
    }
    if (c) {
        free(c);
    }
    return NULL;
}

static int encode_nals(struct x264_ctx *c, struct iovec *pkt,
                       const x264_nal_t *nals, int nnal)
{
    uint8_t *p;
    int i;
    size_t size = c->sei.iov_len;
    struct live_source_ctx *cc = (struct live_source_ctx *)c->parent;

    if (!nnal)
        return 0;

    for (i = 0; i < nnal; i++)
        size += nals[i].i_payload;

    if (c->pkt.iov_len < size) {
        c->pkt.iov_len = size;
        c->pkt.iov_base = realloc(c->pkt.iov_base, size);
    }
    p = c->pkt.iov_base;
    if (!p) {
        return -1;
    }

    pkt->iov_base = p;
    pkt->iov_len = 0;

    static bool append_extradata = false;
    if (!append_extradata) {
        /* Write the extradata as part of the first frame. */
        if (cc->extradata.iov_len > 0 && nnal > 0) {
            if (cc->extradata.iov_len > size) {
                loge("Error: nal buffer is too small\n");
                return -1;
            }
            memcpy(p, cc->extradata.iov_base, cc->extradata.iov_len);
            p += cc->extradata.iov_len;
            pkt->iov_len += cc->extradata.iov_len;
        }
        append_extradata = true;
    }

    for (i = 0; i < nnal; i++){
        memcpy(p, nals[i].p_payload, nals[i].i_payload);
        p += nals[i].i_payload;
        pkt->iov_len += nals[i].i_payload;
    }
    return pkt->iov_len;
}

static int x264_encode(struct x264_ctx *c, struct iovec *in, struct iovec *out)
{
    x264_picture_t pic_out;
    int nNal = 0;
    int ret = 0;
    int i = 0, j = 0;
    c->nal = NULL;
    uint8_t *p422;

    uint8_t *y = c->picture->img.plane[0];
    uint8_t *u = c->picture->img.plane[1];
    uint8_t *v = c->picture->img.plane[2];

    int widthStep422 = c->param->i_width * 2;

    for(i = 0; i < c->param->i_height; i += 2) {
        p422 = (uint8_t *)in->iov_base + i * widthStep422;
        for(j = 0; j < widthStep422; j+=4) {
            *(y++) = p422[j];
            *(u++) = p422[j+1];
            *(y++) = p422[j+2];
        }

        p422 += widthStep422;

        for(j = 0; j < widthStep422; j+=4) {
            *(y++) = p422[j];
            *(v++) = p422[j+3];
            *(y++) = p422[j+2];
        }
    }
    c->picture->i_type = X264_TYPE_I;

    do {
        if (x264_encoder_encode(c->handle, &(c->nal), &nNal, c->picture,
                    &pic_out) < 0) {
            return -1;
        }
        ret = encode_nals(c, out, c->nal, nNal);
        if (ret < 0) {
            return -1;
        }
    } while (0);
    c->picture->i_pts++;
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
    c->width = 640;
    c->height = 480;
    if (c->uvc_opened) {
        logi("uvc already opened!\n");
        return 0;
    }
    c->uvc = uvc_open("/dev/video0", c->width, c->height);
    if (!c->uvc) {
        loge("uvc open failed!\n");
        return -1;
    }
    c->uvc_opened = true;
    ms->opaque = c;
    c->x264 = x264_open(c);
    if (!c->x264) {
        loge("x264_open failed!\n");
        return -1;
    }
    c->data.iov_base = calloc(1, 2*c->width*c->height);
    c->data.iov_len = 2*c->width*c->height;
    return 0;
}

static void live_close(struct media_source *ms)
{
    struct live_source_ctx *c = (struct live_source_ctx *)ms->opaque;
    free(c->data.iov_base);
    x264_close(c->x264);
    uvc_close(c->uvc);
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
    struct live_source_ctx *c = (struct live_source_ctx *)ms->opaque;
    memset(c->data.iov_base, 0, c->data.iov_len);
    int size = uvc_read(c->uvc, c->data.iov_base, c->data.iov_len);
    struct iovec in, out;
    in.iov_base = c->data.iov_base;
    in.iov_len = size;
    x264_encode(c->x264, &in, &out);
    *data = out.iov_base;
    *len = out.iov_len;
    return 0;
}

struct media_source media_source_uvc = {
    .name = "uvc",
    .sdp_generate = sdp_generate,
    .open = live_open,
    .read = live_read,
    .close = live_close,
};
