#include <liblog.h>
#include <libfile.h>
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

struct h264_nalu
{
    const uint8_t* nalu;
    int64_t time;
    long bytes;
    enum ha264_nalu_type type;
};

struct h264_source_ctx {
    int (*init)();
    int (*inject)();
    struct iovec *data;
    struct vector *frame;
    int sps_cnt;
    int duration;

};


static inline const uint8_t* find_start_code(const uint8_t* ptr, const uint8_t* end)
{
    for (const uint8_t *p = ptr; p + 3 < end; p++) {
        if (0x00 == p[0] && 0x00 == p[1] && (0x01 == p[2] || (0x00==p[2] && 0x01==p[3])))
            return p;
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
    bool spspps = true;

    const uint8_t *start = c->data.iov_base;
    const uint8_t *end = start + c->data.iov_len;
    const uint8_t *nalu = search_start_code(start, end);

    while (nalu < end) {
        const unsigned char* nalu2 = search_start_code(nalu + 4, end);
        size_t bytes = nalu2 - nalu;

        int nal_unit_type = h264_nal_type(nalu);
        if (nal_unit_type <= 5) {
            if(c->sps_cnt > 0) spspps = false; // don't need more sps/pps

            struct h264_nalu frame;
            frame.addr = nalu;
            frame.bytes = bytes;
            frame.time = 40 * count++;
            frame.type nal_unit_type;
            vector_push_back(c->frame, nalu);
        } else if(NAL_SPS == nal_unit_type || NAL_PPS == nal_unit_type) {
            if(spspps) {
                c->sps_cnt++;
            }
        }

        nalu = nalu2;
    }

    c->duration = 40 * count;
    return 0;
}

static int h264_file_init(struct h264_source_ctx *c, const char *name)
{
    c->data = file_dump(name);
    if (!c->data) {
        return -1;
    }
    c->frame = vector_create(struct h264_nalu);
    c->sps_cnt = 0;
    h264_parser_frame(c);
    return 0;
}

static void h264_file_deinit(struct h264_source_ctx *c)
{
    vector_destroy(c->frame);
    free(c->data.iov_base);
}




static int get_frame()
{
    return 0;
}


struct media_source h264_media_source = {
    .sdp_generate = sdp_generate,
    .get_frame = get_frame,
};
