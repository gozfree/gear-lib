#include <libmacro.h>
#include "librtp.h"

struct rtp_encode_h264 {
    struct rtp_packet pkt;

};


static void* rtp_h264_pack_create(int size, uint8_t pt, uint16_t seq, uint32_t ssrc, struct rtp_payload_t *handler, void* cbparam)
{
	struct rtp_encode_h264 *packer;
	packer = CALLOC(1, struct rtp_encode_h264);
	if(!packer) return NULL;

	memcpy(&packer->handler, handler, sizeof(packer->handler));
	packer->cbparam = cbparam;
	packer->size = size;

	packer->pkt.rtp.v = RTP_VERSION;
	packer->pkt.rtp.pt = pt;
	packer->pkt.rtp.seq = seq;
	packer->pkt.rtp.ssrc = ssrc;
	return packer;
}

static void rtp_h264_pack_destroy(void* pack)
{
	struct rtp_encode_h264_t *packer;
	packer = (struct rtp_encode_h264_t *)pack;
	free(packer);
}

