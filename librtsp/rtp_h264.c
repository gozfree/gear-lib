#include <libmacro.h>
#include "librtp.h"

static const uint8_t* h264_nalu_find(const uint8_t* p, const uint8_t* end)
{
    for (p += 2; p + 1 < end; p++) {
        if (0x000001 == *(p-2)<<16 | *(p-1)<<8 | *p)
        //if (0x01 == *p && 0x00 == *(p - 1) && 0x00 == *(p - 2))
            return p + 1;
    }
    return end;
}

static int rtp_h264_pack_nalu(struct rtp_encode_h264_t *packer, const uint8_t* nalu, int bytes)
{
    int n;
    uint8_t *rtp;

    packer->pkt.payload = nalu;
    packer->pkt.payloadlen = bytes;
    n = RTP_FIXED_HEADER + packer->pkt.payloadlen;
    rtp = (uint8_t*)packer->handler.alloc(packer->cbparam, n);
    if (!rtp) return ENOMEM;

    //packer->pkt.rtp.m = 1; // set marker flag
    packer->pkt.rtp.m = (*nalu & 0x1f) <= 5 ? 1 : 0; // VCL only
    n = rtp_packet_serialize(&packer->pkt, rtp, n);
    if (n != RTP_FIXED_HEADER + packer->pkt.payloadlen)
    {
            assert(0);
            return -1;
    }

    ++packer->pkt.rtp.seq;
    packer->handler.packet(packer->cbparam, rtp, n, packer->pkt.rtp.timestamp, 0);
    packer->handler.free(packer->cbparam, rtp);
    return 0;
}

static int rtp_h264_pack_fu_a(struct rtp_encode_h264_t *packer, const uint8_t* nalu, int bytes)
{
	int n;
	unsigned char *rtp;

	// RFC6184 5.3. NAL Unit Header Usage: Table 2 (p15)
	// RFC6184 5.8. Fragmentation Units (FUs) (p29)
	uint8_t fu_indicator = (*nalu & 0xE0) | 28; // FU-A
	uint8_t fu_header = *nalu & 0x1F;

	nalu += 1; // skip NAL Unit Type byte
	bytes -= 1;
	assert(bytes > 0);

	// FU-A start
	for (fu_header |= FU_START; bytes > 0; ++packer->pkt.rtp.seq)
	{
		if (bytes + RTP_FIXED_HEADER <= packer->size - N_FU_HEADER)
		{
			assert(0 == (fu_header & FU_START));
			fu_header = FU_END | (fu_header & 0x1F); // FU-A end
			packer->pkt.payloadlen = bytes;
		}
		else
		{
			packer->pkt.payloadlen = packer->size - RTP_FIXED_HEADER - N_FU_HEADER;
		}

		packer->pkt.payload = nalu;
		n = RTP_FIXED_HEADER + N_FU_HEADER + packer->pkt.payloadlen;
		rtp = (uint8_t*)packer->handler.alloc(packer->cbparam, n);
		if (!rtp) return -ENOMEM;

		packer->pkt.rtp.m = (FU_END & fu_header) ? 1 : 0; // set marker flag
		n = rtp_packet_serialize_header(&packer->pkt, rtp, n);
		if (n != RTP_FIXED_HEADER)
		{
			assert(0);
			return -1;
		}

		/*fu_indicator + fu_header*/
		rtp[n + 0] = fu_indicator;
		rtp[n + 1] = fu_header;
		memcpy(rtp + n + N_FU_HEADER, packer->pkt.payload, packer->pkt.payloadlen);

		packer->handler.packet(packer->cbparam, rtp, n + N_FU_HEADER + packer->pkt.payloadlen, packer->pkt.rtp.timestamp, 0);
		packer->handler.free(packer->cbparam, rtp);

		bytes -= packer->pkt.payloadlen;
		nalu += packer->pkt.payloadlen;
		fu_header &= 0x1F; // clear flags
	}

	return 0;
}

int rtp_payload_h264_encode(struct rtp_packet *pkt, const void* h264, int bytes, uint32_t timestamp)
{
    int r = 0;
    const uint8_t *p1, *p2, *pend;
    pkt.rtp.timestamp = timestamp;

    pend = (const uint8_t*)h264 + bytes;
    for (p1 = h264_nalu_find((const uint8_t*)h264, pend); p1 < pend && 0 == r; p1 = p2) {
        size_t nalu_size;

        // filter H.264 start code(0x00000001)
        assert(0 < (*p1 & 0x1F) && (*p1 & 0x1F) < 24);
        p2 = h264_nalu_find(p1 + 1, pend);
        nalu_size = p2 - p1;

        // filter suffix '00' bytes
        if (p2 != pend) --nalu_size;
        while(0 == p1[nalu_size-1]) --nalu_size;

        if(nalu_size + RTP_FIXED_HEADER <= (size_t)packer->size)
        {
                // single NAl unit packet 
                r = rtp_h264_pack_nalu(packer, p1, nalu_size);
        }
        else
        {
                r = rtp_h264_pack_fu_a(packer, p1, nalu_size);
        }
    }

    return 0;
}
