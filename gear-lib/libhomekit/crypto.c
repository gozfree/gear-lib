#include <arpa/inet.h>

#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/hmac.h>

#include "crypto.h"

#define RTCP_FIR 192
#define RTCP_IJ 195
#define RTCP_SR 200
#define RTCP_TOKEN 210

#define RTP_PT_IS_RTCP(x) (((x) >= RTCP_FIR && (x) <= RTCP_IJ) || \
                           ((x) >= RTCP_SR  && (x) <= RTCP_TOKEN))


static void encrypt_counter(Aes *aes, uint8_t *iv, uint8_t *outbuf, int outlen) {
    int i, j, outpos;
    for (i = 0, outpos = 0; outpos < outlen; i++) {
        uint8_t keystream[16];
        iv[15] = i & 0xff;
        iv[14] = (i >> 8) & 0xff;

        wc_AesEncryptDirect(aes, keystream, iv);
        for (j = 0; j < 16 && outpos < outlen; j++, outpos++)
            outbuf[outpos] ^= keystream[j];
    }
}


static void derive_key(Aes *aes, const uint8_t *salt, int label, uint8_t *out, int outlen) {
    uint8_t input[16] = { 0 };
    memcpy(input, salt, 14);
    // Key derivation rate assumed to be zero
    input[14 - 7] ^= label;
    memset(out, 0, outlen);
    encrypt_counter(aes, input, out, outlen);
}


static void create_iv(uint8_t *iv, const uint8_t *salt, uint64_t index, uint32_t ssrc) {
    memset(iv, 0, 16);
    *((uint32_t*)(iv + 4)) = htonl(ssrc);

    uint8_t indexbuf[8];
    *((uint32_t*)indexbuf) = htonl((index >> 32) & 0xffffffff);
    *((uint32_t*)(indexbuf + 4)) = htonl(index & 0xffffffff);

    int i;
    for (i = 0; i < 8; i++) // index << 16
        iv[6 + i] ^= indexbuf[i];

    for (i = 0; i < 14; i++)
        iv[i] ^= salt[i];
}


void session_init_crypto(streaming_session_t *session) {
    Aes *aes = malloc(sizeof(Aes));
    wc_AesInit(aes, NULL, INVALID_DEVID);
    wc_AesSetKey(
        aes,
        session->settings->srtp_video_master_key, session->settings->srtp_video_master_key_size,
        (uint8_t*)"0123456789abcdef", AES_ENCRYPTION
    );

    derive_key(aes, session->settings->srtp_video_master_salt, 0x00,
               session->video_rtp.key, sizeof(session->video_rtp.key));
    derive_key(aes, session->settings->srtp_video_master_salt, 0x02,
               session->video_rtp.salt, sizeof(session->video_rtp.salt));
    derive_key(aes, session->settings->srtp_video_master_salt, 0x01,
               session->video_rtp.auth, sizeof(session->video_rtp.auth));

    derive_key(aes, session->settings->srtp_video_master_salt, 0x03,
               session->video_rtcp.key, sizeof(session->video_rtcp.key));
    derive_key(aes, session->settings->srtp_video_master_salt, 0x05,
               session->video_rtcp.salt, sizeof(session->video_rtcp.salt));
    derive_key(aes, session->settings->srtp_video_master_salt, 0x04,
               session->video_rtcp.auth, sizeof(session->video_rtcp.auth));

    wc_AesFree(aes);
    free(aes);
}


int session_encrypt(streaming_session_t *session, uint8_t *buffer, int len, int buffer_size) {
    if (len < 8)
        return -1;

    int rtcp = RTP_PT_IS_RTCP(buffer[1]);
    int hmac_size = 10; //rtcp ? s->rtcp_hmac_size : s->rtp_hmac_size;
    int padding = hmac_size;
    if (rtcp)
        padding += 4; // For the RTCP index

    if (len + padding > buffer_size) {
        return -1;
    }

    uint8_t iv[16] = { 0 }, hmac_data[20];

    uint8_t *buf = buffer;

    uint64_t index;
    uint32_t ssrc;

    if (rtcp) {
        // TODO:
        ssrc = *((uint32_t*)(buf + 4));
        index = session->rtcp_index++;

        buf += 8;
        len -= 8;
    } else {
        int seq = ntohs(*((uint16_t*)(buf + 2)));

        if (len < 12) {
            return -1;
        }

        ssrc = ntohl(*((uint32_t*)(buf + 8)));

        if (seq < session->sequence_largest)
            session->roc++;
        session->sequence_largest = seq;
        index = seq + (((uint64_t)session->roc) << 16);

        buf += 12;
        len -= 12;
    }

    create_iv(iv, rtcp ? session->video_rtcp.salt : session->video_rtp.salt, index, ssrc);

    Aes *aes = (Aes*) malloc(sizeof(Aes));
    wc_AesInit(aes, NULL, INVALID_DEVID);
    wc_AesSetKey(
        aes,
        rtcp ? session->video_rtcp.key : session->video_rtp.key, 
        rtcp ? sizeof(session->video_rtcp.key) : sizeof(session->video_rtp.key),
        NULL, AES_ENCRYPTION
    );
    encrypt_counter(aes, iv, buf, len);

    wc_AesFree(aes);
    free(aes);

    if (rtcp) {
        *((uint32_t*)(buf + len)) = htonl(0x80000000 | index);
        len += 4;
    }

    Hmac *hmac = (Hmac*) malloc(sizeof(Hmac));
    wc_HmacInit(hmac, NULL, INVALID_DEVID);
    wc_HmacSetKey(
        hmac, WC_SHA, 
        rtcp ? session->video_rtcp.auth : session->video_rtp.auth,
        rtcp ? sizeof(session->video_rtcp.auth) : sizeof(session->video_rtp.auth)
    );

    wc_HmacUpdate(hmac, buffer, buf + len - buffer);

    if (!rtcp) {
        uint8_t rocbuf[4];
        *((uint32_t*)rocbuf) = session->roc;
        wc_HmacUpdate(hmac, rocbuf, 4);
    }

    wc_HmacFinal(hmac, hmac_data);

    wc_HmacFree(hmac);
    free(hmac);

    memcpy(buf + len, hmac_data, hmac_size);
    len += hmac_size;
    return buf + len - buffer;
}


