#include <liblog.h>
#include <libfile.h>
#include <libmacro.h>
#include <libvector.h>
#include "sdp.h"
#include "media_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

enum h264_nalu_type {
    NAL_SLICE           = 1,
    NAL_DPA             = 2,
    NAL_DPB             = 3,
    NAL_DPC             = 4,
    NAL_IDR_SLICE       = 5,
    NAL_SEI             = 6,
    NAL_SPS             = 7,
    NAL_PPS             = 8,
    NAL_AUD             = 9,
    NAL_END_SEQUENCE    = 10,
    NAL_END_STREAM      = 11,
    NAL_FILLER_DATA     = 12,
    NAL_SPS_EXT         = 13,
    NAL_AUXILIARY_SLICE = 19,
    NAL_FF_IGNORE       = 0xff0f001,
};

#define H264_NAL(v)	(v & 0x1F)

struct h264_frame
{
    const uint8_t* addr;
    int64_t time;
    long bytes;
    enum h264_nalu_type type;
};

struct h264_media_source_handle {
    int (*init)();
    int (*injector)();
    void (*deinit)();
};

struct h264_source_ctx {
    const char name[32];
    struct iovec *data;
    struct vector *frame;
    vector_iter vit;
    int sps_cnt;
    int duration;
};


static inline const uint8_t* find_start_code(const uint8_t* ptr, const uint8_t* end)
{
    for (const uint8_t *p = ptr; p + 3 < end; p++) {
        if (0x00 == p[0] && 0x00 == p[1] && (0x01 == p[2] || (0x00==p[2] && 0x01==p[3]))) {
            return p;
        }
    }
    return end;
}

static inline int h264_nal_type(const unsigned char* ptr)
{
    int i = 2;
    if (!(0x00 == ptr[0] && 0x00 == ptr[1])) {
        return -1;
    }
    if (0x00 == ptr[2])
        ++i;
    if (0x01 != ptr[i]) {
        return -1;
    }
    return H264_NAL(ptr[i+1]);
}

static int h264_parser_frame(struct h264_source_ctx *c)
{
    size_t count = 0;
//    bool spspps = true;

    const uint8_t *start = c->data->iov_base;
    const uint8_t *end = start + c->data->iov_len;
    const uint8_t *nalu = find_start_code(start, end);

    while (nalu < end) {
        const unsigned char* nalu2 = find_start_code(nalu + 4, end);
        size_t bytes = nalu2 - nalu;

        int nal_unit_type = h264_nal_type(nalu);

        struct h264_frame frame;
        frame.addr = nalu;
        frame.bytes = bytes;
        frame.time = 90000 * count++;
        frame.type = nal_unit_type;
        vector_push_back(c->frame, frame);
#if 0
        if (nal_unit_type <= 5) {
            if(c->sps_cnt > 0) spspps = false; // don't need more sps/pps

            struct h264_frame frame;
            frame.addr = nalu;
            frame.bytes = bytes;
            frame.time = 40 * count++;
            frame.type = nal_unit_type;
            vector_push_back(c->frame, frame);
        } else if(NAL_SPS == nal_unit_type || NAL_PPS == nal_unit_type) {
            if(spspps) {
                c->sps_cnt++;
            }
        }
#endif

        nalu = nalu2;
    }

    c->duration = 40 * count;
    return 0;
}

static int h264_file_open(struct media_source *ms, const char *name)
{
    struct h264_source_ctx *c = CALLOC(1, struct h264_source_ctx);
    c->data = file_dump(name);
    if (!c->data) {
        loge("file_dump %s failed!\n", name);
        return -1;
    }
    c->frame = vector_create(struct h264_frame);
    c->sps_cnt = 0;
    h264_parser_frame(c);
    c->vit = vector_begin(c->frame);
    ms->opaque = c;
    return 0;
}

static void h264_file_close(struct media_source *ms)
{
    struct h264_source_ctx *c = (struct h264_source_ctx *)ms->opaque;
    vector_destroy(c->frame);
    free(c->data->iov_base);
}

static int h264_file_read_frame(struct media_source *ms, void **data, size_t *len)
{
    struct h264_source_ctx *c = (struct h264_source_ctx *)ms->opaque;
    struct h264_frame *frame;
    if (c->vit == NULL || c->vit == vector_end(c->frame)) {
        return -1;
    }
    frame = vector_iter_valuep(c->frame, c->vit, struct h264_frame);
    *data = (void *)frame->addr;
    *len = frame->bytes;
    c->vit = vector_next(c->frame);
    return 0;
}

static int h264_file_write(struct media_source *ms, void *data, size_t len)
{
    struct h264_source_ctx *c = (struct h264_source_ctx *)ms->opaque;
    c = c;

    return 0;
}

static uint32_t get_random_number()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    srand(now.tv_usec);
    return (rand() % ((uint32_t)-1));
}

static int is_auth()
{
    return 0;
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


struct media_source media_source_h264 = {
    .name = "H264",
    .sdp_generate = sdp_generate,
    .open = h264_file_open,
    .read = h264_file_read_frame,
    .write = h264_file_write,
    .close = h264_file_close,
};
