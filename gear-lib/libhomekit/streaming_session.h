#pragma once


#include <sys/socket.h>
#include "camera_session.h"


typedef struct {
    uint8_t key[16];
    uint8_t salt[14];
    uint8_t auth[20];
} srtp_keys_t;


typedef struct _streaming_session {
    struct sockaddr_in controller_addr;

    bool started;
    bool failed;

    uint32_t timestamp;
    uint16_t sequence;

    int sequence_largest;
    uint32_t rtcp_index;
    uint32_t roc;
    uint8_t buffered_nals;

    uint8_t *video_buffer;
    uint8_t *video_buffer_ptr;

    srtp_keys_t video_rtp;
    srtp_keys_t video_rtcp;

    /*
    srtp_keys_t audio_rtp;
    srtp_keys_t audio_rtcp;
    */

    camera_session_t *settings;

    struct _streaming_session *next;
} streaming_session_t;
