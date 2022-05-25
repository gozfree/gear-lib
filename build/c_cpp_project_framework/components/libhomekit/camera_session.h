#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <homekit/homekit.h>

#define MAX_CAMERA_SESSIONS 4


typedef enum {
    IP_VERSION_IPV4 = 0,
    IP_VERSION_IPV6 = 1
} ip_version_t;


typedef enum {
    SRTP_CRYPTO_AES_CM_128_HMAC_SHA1_80 = 0,
    SRTP_CRYPTO_AES_256_CM_HMAC_SHA1_80 = 1,
    SRTP_CRYPTO_DISABLED = 2
} srtp_crypto_suite_t;


typedef struct _camera_session_t {
    homekit_client_id_t client_id;

    char session_id[17];
    uint8_t status;

    ip_version_t controller_ip_version;
    char *controller_ip_address;
    uint16_t controller_video_port;
    uint16_t controller_audio_port;

    srtp_crypto_suite_t srtp_video_crypto_suite;
    uint8_t srtp_video_master_key[33];
    size_t srtp_video_master_key_size;
    uint8_t srtp_video_master_salt[15];
    size_t srtp_video_master_salt_size;

    srtp_crypto_suite_t srtp_audio_crypto_suite;
    uint8_t srtp_audio_master_key[33];
    size_t srtp_audio_master_key_size;
    uint8_t srtp_audio_master_salt[15];
    size_t srtp_audio_master_salt_size;

    uint32_t video_ssrc;
    uint32_t audio_ssrc;

    uint8_t video_rtp_payload_type;
    uint16_t video_rtp_max_bitrate;
    float video_rtp_min_rtcp_interval;
    uint16_t video_rtp_max_mtu;

    bool active;
    bool started;

    struct _camera_session_t *next;
} camera_session_t;


camera_session_t *camera_session_new();
void camera_session_free(camera_session_t *session);
