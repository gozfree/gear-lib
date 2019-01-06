/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#include "rtmp_aac.h"
#include "rtmp_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct aac_adts_header
{
    /* Byte 1 low bit to high bit*/
    uint8_t syncword_12to5        :8;

    /* Byte 2 low bit to high bit*/
    uint8_t protection            :1;
    uint8_t layer                 :2;
    uint8_t mpeg_version          :1;
    uint8_t syncword_4to1         :4;

    /* Byte 3 low bit to high bit*/
    uint8_t channel_conf_3        :1;
    uint8_t private_stream        :1;
    uint8_t sample_freqency_index :4;
    uint8_t profile               :2;

    /* Byte 4 low bit to high bit*/
    uint8_t framelength_13to12    :2;
    uint8_t copyright_start       :1;
    uint8_t copyrighted_stream    :1;
    uint8_t home                  :1;
    uint8_t originality           :1;
    uint8_t channel_conf_2to1     :2;

    /* Byte 5 low bit to high bit*/
    uint8_t framelength_11to4     :8;

    /* Byte 6 low bit to high bit*/
    uint8_t buffer_fullness_11to7 :5;
    uint8_t framelength_3to1      :3;

    /* Byte 7 low bit to high bit*/
    uint8_t number_of_aac_frame   :2;
    uint8_t buffer_fullness_6to1  :6;
};

static bool is_sync_word_ok(struct aac_adts_header *h)
{
    return (0x0FFF == (0x0000 | (h->syncword_12to5 << 4) | h->syncword_4to1));
}

static uint16_t frame_length(struct aac_adts_header *h)
{
    return (uint16_t) (0x0000 | (h->framelength_13to12 << 11)
      | (h->framelength_11to4 << 3) | (h->framelength_3to1));
}

int aac_add(struct rtmp *rtmp, struct iovec *data)
{
    rtmp->audio = (struct rtmp_audio_params *)calloc(1, sizeof(struct rtmp_audio_params));
    rtmp->audio->codec_id = RTMP_DATA_AAC;
    rtmp->audio->bitrate = 0;
    rtmp->audio->sample_size = 0;
    rtmp->audio->sample_rate = 0;
    rtmp->audio->channels = 0;

    static const int ff_mpeg4audio_sample_rates[16] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 7350,
        0,0,0
    };

    int index;
    for (index = 0; index < 16; index++)
        if (rtmp->audio->sample_rate == ff_mpeg4audio_sample_rates[index])
            break;
    if (index == 16) {
            printf("Unsupported sample rate %d\n",rtmp->audio->sample_rate);
            return -1;
    }
    rtmp->audio->extra_data_len = 2;
    rtmp->audio->extra_data = calloc(1, 16);
    if (!rtmp->audio->extra_data){
            printf("Failed to alloc extra_data\n");
            return -1;
    }
    rtmp->audio->extra_data[0] = 0x02 << 3 | index >> 1;
    rtmp->audio->extra_data[1] = (index & 0x01) << 7 | rtmp->audio->channels << 3;
    return 0;
}

int aac_write_header(struct rtmp *rtmp)
{
    struct rtmp_private_buf *buf = rtmp->buf;
    unsigned int timestamp = 0;
    put_byte(buf, FLV_TAG_TYPE_AUDIO);
    put_be24(buf, 0); //size patched later
    put_be24(buf, timestamp); // ts
    put_byte(buf, (timestamp >> 24)&0x7f); // ts ext, timestamp is signed-32 bit
    put_be24(buf, 0); // streamid

    unsigned int pos = tell(buf);
    int flags = FLV_CODECID_AAC | FLV_SAMPLERATE_44100HZ | FLV_SAMPLESSIZE_16BIT | FLV_STEREO;
    put_byte(buf, flags);
    put_byte(buf, 0);//AAC sequence header
    append_data(buf, rtmp->audio->extra_data, rtmp->audio->extra_data_len);

    unsigned int data_size = tell(buf) - pos;
    update_amf_be24(buf, data_size,pos - 10);
    put_be32(buf, data_size + 11); // Last tag size
    return 0;
}

int aac_write_packet(struct rtmp *rtmp, struct rtmp_packet *pkt)
{
    struct rtmp_private_buf *buf = rtmp->buf;
    put_byte(buf, FLV_TAG_TYPE_AUDIO);
    put_be24(buf, pkt->len + 2);
    put_be24(buf, pkt->timestamp);
    put_byte(buf, (pkt->timestamp >> 24)&0x7f);
    put_be24(buf, 0);//streamId, always 0

    int flags = FLV_CODECID_AAC | FLV_SAMPLERATE_44100HZ | FLV_SAMPLESSIZE_16BIT | FLV_STEREO;
    put_byte(buf, flags);
    put_byte(buf, 1);// AAC raw
    append_data(buf, pkt->data, pkt->len);

    put_be32(buf, 11 + pkt->len  + 2);

    if (flush_data(rtmp, 1)) {
        return -1;
    }
    return 0;
}

int aac_send_data(struct rtmp *rtmp, uint8_t *data, int len, uint32_t timestamp)
{
#define ADTS_AU_HEADER_LEN  7
    if (len <= ADTS_AU_HEADER_LEN) {
        printf("AAC data length is invalid\n");
        return -1;
    }
    //if aac_frame_len != len, aac data contain several frames,
    // need split to strip header
    int frame_len = 0;
    int sent_len = 0;
    uint8_t *frame_ptr = data;

    while (sent_len < len) {
        struct aac_adts_header *adts = (struct aac_adts_header *)frame_ptr;
        if (is_sync_word_ok(adts)) {
            frame_len = frame_length(adts);
            struct item *pkt = item_alloc(rtmp->q, frame_ptr + 7, frame_len - 7);
            if (!pkt) {
                queue_push(rtmp->q, pkt);
            } else {
                printf("alloc pkt failed!\n");
            }
            frame_ptr += frame_len;
            sent_len += frame_len;
        } else {
            printf("is_sync_word_error\n");
            frame_ptr++;
            sent_len++;
        }
    }
    return 0;
}
