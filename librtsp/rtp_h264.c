#include "rtp.h"
#include <liblog.h>
#include <libmacro.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Generic Nal header
 * +-+-+-+-+-+-+-+-+
 * |7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+
 * |F|NRI|  Type   |
 * +-+-+-+-+-+-+-+-+
 */

struct h264_nal_header
{
#ifdef BIGENDIAN
    uint8_t f    :1; /* Bit 7     */
    uint8_t nri  :2; /* Bit 6 ~ 5 */
    uint8_t type :5; /* Bit 4 ~ 0 */
#else
    uint8_t type :5; /* Bit 0 ~ 4 */
    uint8_t nri  :2; /* Bit 5 ~ 6 */
    uint8_t f    :1; /* Bit 7     */
#endif
};

typedef struct h264_nal_header H264_NAL_HEADER;
typedef struct h264_nal_header H264_FU_INDICATOR;
typedef struct h264_nal_header H264_STAP_HEADER;
typedef struct h264_nal_header H264_MTAP_HEADER;

/*  FU header
 * +-+-+-+-+-+-+-+-+
 * |7|6|5|4|3|2|1|0|
 * +-+-+-+-+-+-+-+-+
 * |S|E|R|  Type   |
 * +-+-+-+-+-+-+-+-+
 */

struct h264_fu_header
{
#ifdef BIGENDIAN
    uint8_t s    :1; /* Bit 7     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t r    :1; /* Bit 5     */
    uint8_t type :5; /* Bit 4 ~ 0 */
#else
    uint8_t type :5; /* Bit 0 ~ 4 */
    uint8_t r    :1; /* Bit 5     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t s    :1; /* Bit 7     */
#endif
};

typedef struct h264_fu_header H264_FU_HEADER;

#define FU_START    0x80
#define FU_END      0x40
#define N_FU_HEADER 2

static const uint8_t* h264_nalu_find(const uint8_t* p, const uint8_t* end)
{
    int j = 0;
    for (p += 2; p + 1 < end; p++, j++) {
        //if (0x000001 == (*(p-2)<<16 | *(p-1)<<8 | *p)) {
        if (0x01 == *p && 0x00 == *(p - 1) && 0x00 == *(p - 2)) {
            //loge("got nalu, j=%d\n", j);
            return p + 1;
        }
    }
    return end;
}

static int rtp_h264_pack_nalu(struct rtp_socket *sock, struct rtp_packet *pkt, const uint8_t* nalu, int bytes)
{
    int n;
    uint8_t *rtp;

    pkt->payload = nalu;
    pkt->payloadlen = bytes;
    n = RTP_FIXED_HEADER + pkt->payloadlen;
    rtp = (uint8_t*)calloc(1, n);
    if (!rtp) return -1;

    pkt->header.m = (*nalu & 0x1f) <= 5 ? 1 : 0; // VCL only
    n = rtp_packet_serialize(pkt, rtp, n);
    if (n != RTP_FIXED_HEADER + pkt->payloadlen) {
        return -1;
    }

    ++pkt->header.seq;
    rtp_sendto(sock, NULL, 0, rtp, n);
    free(rtp);
    return 0;
}

static int rtp_h264_pack_fu_a(struct rtp_socket *sock, struct rtp_packet *pkt, const uint8_t* nalu, int bytes)
{
    int n;
    unsigned char *rtp;

    uint8_t fu_indicator = (*nalu & 0xE0) | 28; // FU-A
    uint8_t fu_header = *nalu & 0x1F;

    nalu += 1; // skip NAL Unit Type byte
    bytes -= 1;

    // FU-A start
    for (fu_header |= FU_START; bytes > 0; ++pkt->header.seq) {
        if (bytes + RTP_FIXED_HEADER <= /*pkt->size*/1448 - N_FU_HEADER) {
            if (0 != (fu_header & FU_START)) {
                return -1;
            }
            fu_header = FU_END | (fu_header & 0x1F); // FU-A end
            pkt->payloadlen = bytes;
        } else {
            pkt->payloadlen = /*pkt->size*/1448 - RTP_FIXED_HEADER - N_FU_HEADER;
        }
        pkt->payload = nalu;
        n = RTP_FIXED_HEADER + N_FU_HEADER + pkt->payloadlen;
        rtp = (uint8_t*)calloc(1, n);
        if (!rtp) return -1;

        pkt->header.m = (FU_END & fu_header) ? 1 : 0; // set marker flag
        n = rtp_packet_serialize_header(pkt, rtp, n);
        if (n != RTP_FIXED_HEADER) {
            return -1;
        }

        /*fu_indicator + fu_header*/
        rtp[n + 0] = fu_indicator;
        rtp[n + 1] = fu_header;
        memcpy(rtp + n + N_FU_HEADER, pkt->payload, pkt->payloadlen);
        rtp_sendto(sock, NULL, 0, rtp, n + N_FU_HEADER + pkt->payloadlen);
        free(rtp);

        bytes -= pkt->payloadlen;
        nalu += pkt->payloadlen;
        fu_header &= 0x1F; // clear flags
    }


    return 0;
}

int rtp_payload_h264_encode(struct rtp_socket *sock, struct rtp_packet *pkt, const void* h264, int bytes, uint32_t timestamp)
{
    int r = 0;
    const uint8_t *p1, *p2, *pend;
    pkt->header.timestamp = timestamp;

    pend = (const uint8_t*)h264 + bytes;
    for (p1 = h264_nalu_find((const uint8_t*)h264, pend); p1 < pend && 0 == r; p1 = p2) {
        size_t nalu_size;

        p2 = h264_nalu_find(p1 + 1, pend);
        nalu_size = p2 - p1;

        // filter suffix '00' bytes
        if (p2 != pend) --nalu_size;
        while(0 == p1[nalu_size-1]) --nalu_size;

        if (nalu_size + RTP_FIXED_HEADER <= (size_t)pkt->size) {
            r = rtp_h264_pack_nalu(sock, pkt, p1, nalu_size);
        } else {
            r = rtp_h264_pack_fu_a(sock, pkt, p1, nalu_size);
        }
    }
    return 0;
}
