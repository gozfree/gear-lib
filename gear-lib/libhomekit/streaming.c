#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <time.h>

#if 0
#include <esp_log.h>
#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_camera.h>
#endif
#include <x264.h>
#include <jpeglib.h>

#include "config.h"
#include "debug.h"
#include "camera_session.h"
#include "crypto.h"
#include "streaming.h"

#define ESP_LOGI(tag, format, ... ) printf(format, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ... ) printf(format, ##__VA_ARGS__)

#define vTaskDelay sleep


#define RTCP_SR 200

#define RTP_VERSION 2
#define RTCP_VERSION 2
#define RTP_MAX_PACKET_LENGTH 2048 // 8192


#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)

#define HMAC_SIZE 10


typedef struct {
    uint8_t reception_report_count:5;
    uint8_t padding:1;
    uint8_t version:2;
    uint8_t payload_type;
    uint16_t length;
    uint32_t sender_ssrc;
} rtcp_header_t;


typedef struct {
    uint32_t ntp_timestamp_seconds;
    uint32_t ntp_timestamp_picoseconds;
    uint32_t rtp_timestamp;
    uint32_t sender_packet_count;
    uint32_t sender_octet_count;
} rtcp_sender_report_t;


typedef struct {
    unsigned int cc:4;         /* CSRC count */
    unsigned int extension:1;  /* header extension flag */
    unsigned int padding:1;    /* padding flag */
    unsigned int version:2;    /* protocol version */
    unsigned int payload_type:7;  /* payload type */
    unsigned int marker:1;     /* marker bit */
    unsigned int seq:16;       /* sequence number */
    uint32_t timestamp;        /* timestamp */
    uint32_t ssrc;             /* synchronization source */
    uint32_t csrc[];           /* optional CSRC list */
} rtp_header_t;


uint64_t ntp_get_time() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL)) {
        ESP_LOGE(TAG, "Failed to get current time for RTCP Sender Report");
        return -1;
    }
    return ((((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec) / 1000) * 1000 + NTP_OFFSET_US;
}


uint32_t get_time_millis() {
#if 0
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#else
    return 0;
#endif
}


uint8_t* find_nal_start(uint8_t* start, uint8_t* end) {
    if (start >= end)
        return end;

    uint8_t* p = start;
    uint8_t state = 0;
    while (p < end) {
        switch (state) {
        case 0:
            if (*p == 0)
                state++;
            break;
        case 1:
            if (*p == 0) {
                state++;
            } else {
                state = 0;
            }
            break;
        case 2:
            if (*p == 0) {
                state++;
            } else if (*p == 1) {
                return p - 2;
            } else {
                state = 0;
            }
            break;
        case 3:
            if (*p == 1) {
                return p - 3;
            } else if (*p != 0) {
                state = 0;
            }
            break;
        }

        p++;
    }

    return p;
}


static int send_rtcp_sender_report(streaming_session_t *session) {
    uint8_t *buffer = (uint8_t*) malloc(RTP_MAX_PACKET_LENGTH);

    rtcp_header_t *rtcp_header = (rtcp_header_t*) buffer;
    memset(rtcp_header, 0, sizeof(*rtcp_header));
    rtcp_header->version = RTCP_VERSION;
    rtcp_header->payload_type = RTCP_SR;
    rtcp_header->length = htons((sizeof(rtcp_header_t) + sizeof(rtcp_sender_report_t) + 3) / 4 - 1);
    rtcp_header->sender_ssrc = htonl(session->settings->video_ssrc);

    rtcp_sender_report_t *sender_report = (rtcp_sender_report_t*) (buffer + sizeof(rtcp_header_t));
    uint64_t ntp_time = ntp_get_time();
    sender_report->ntp_timestamp_seconds = htonl(ntp_time / 1000000);
    sender_report->ntp_timestamp_picoseconds = htonl(((ntp_time % 1000000) << 32) / 1000000);
    sender_report->rtp_timestamp = htonl(session->timestamp);
    sender_report->sender_packet_count = 0;
    sender_report->sender_octet_count = 0;

    // dump_binary("RTCP Sender Report: ", buffer, sizeof(rtcp_header_t) + sizeof(rtcp_sender_report_t));

    int encrypted_size = session_encrypt(session, buffer, sizeof(rtcp_header_t) + sizeof(rtcp_sender_report_t), RTP_MAX_PACKET_LENGTH);
    if (encrypted_size < 0) {
        ESP_LOGE(TAG, "Failed to encrypt RTCP payload (code %d)", encrypted_size);
        free(buffer);
        return -1;
    }

    ESP_LOGI(TAG, "Sending RTCP sender report (%d bytes)", encrypted_size);
    int r = send(session->settings->video_socket, buffer, encrypted_size, 0);
    if (r < 0) {
        ESP_LOGE(TAG, "Failed to send RTCP sender report packet (code %d)", r);
        free(buffer);
        return -1;
    }

    free(buffer);

    return 0;
}


static int session_send_data(streaming_session_t *session, bool last) {
    size_t size = session->video_buffer_ptr - session->video_buffer;
    if (size <= sizeof(rtp_header_t))
        return 0;

    rtp_header_t *header = (rtp_header_t*) session->video_buffer;
    header->version = RTP_VERSION;
    header->payload_type = session->settings->video_rtp_payload_type;
    header->ssrc = htonl(session->settings->video_ssrc);
    header->timestamp = htonl(session->timestamp);
    header->marker = last;
    header->seq = htons(session->sequence);

    session->sequence = (session->sequence + 1) & 0xffff;

    // dump_binary("Sending data: ", session->video_buffer, (size > 100) ? 100 : size);
    // dump_binary("Sending data: ", session->video_buffer, size);

    int encrypted_size = session_encrypt(session, session->video_buffer, size, RTP_MAX_PACKET_LENGTH);
    if (encrypted_size < 0) {
        ESP_LOGE(TAG, "Failed to encrypt RTP payload (code %d)", encrypted_size);
        return -1;
    }

    ESP_LOGI(TAG, "Sending %d bytes to socket %d", encrypted_size, session->settings->video_socket);
#if 0
    ESP_LOGI(TAG, "Free heap: %d", xPortGetFreeHeapSize());
#endif
    int r = send(session->settings->video_socket, session->video_buffer, encrypted_size, 0);
    if (r < 0) {
        ESP_LOGE(TAG, "Failed to send RTP packet (code %d)", r);
        return -1;
    }

    session->video_buffer_ptr = session->video_buffer + sizeof(rtp_header_t);

    return 0;
}


static int session_flush_buffered(streaming_session_t *session, bool last) {
    size_t size = session->video_buffer_ptr - session->video_buffer;
    if (size == 0)
        return 0;

    if (session->buffered_nals == 1) {
        memmove(session->video_buffer + sizeof(rtp_header_t),
                session->video_buffer + sizeof(rtp_header_t) + 3,
                size - 3);
        session->video_buffer_ptr -= 3;
    }
    if (session_send_data(session, last))
        return -1;

    session->buffered_nals = 0;
    return 0;
}


static int session_send_nal(streaming_session_t *session, uint8_t *nal, size_t nal_size, bool last) {
    int r;
    if (nal_size <= session->settings->video_rtp_max_mtu) {
        int buffered_size = session->video_buffer_ptr - session->video_buffer - sizeof(rtp_header_t);

        if (buffered_size + 2 + nal_size > session->settings->video_rtp_max_mtu) {
            r = session_flush_buffered(session, false);
            if (r)
                return -1;
            buffered_size = 0;
        }

        if (buffered_size + 3 + nal_size <= session->settings->video_rtp_max_mtu) {
            if (buffered_size == 0) {
                *(session->video_buffer_ptr++) = 24;  // STAP-A
            }
            *(session->video_buffer_ptr++) = (nal_size >> 8) & 0xff;
            *(session->video_buffer_ptr++) = nal_size & 0xff;

            memcpy(session->video_buffer_ptr, nal, nal_size);
            session->video_buffer_ptr += nal_size;
            session->buffered_nals++;
        } else {
            r = session_flush_buffered(session, false);
            if (r)
                return -1;

            memcpy(session->video_buffer_ptr, nal, nal_size);
            session->video_buffer_ptr += nal_size;

            r = session_send_data(session, last);
            if (r)
                return -1;
        }
    } else {
        r = session_flush_buffered(session, false);
        if (r)
            return -1;

        uint8_t type = nal[0] & 0x1F;
        uint8_t nri = nal[0] & 0x60;

        uint8_t *nal_end = nal + nal_size;
        nal += 1;

        size_t fragment_header_size = 2;
        size_t max_fragment_payload_size = session->settings->video_rtp_max_mtu - fragment_header_size;

        bool first_fragment = true;

        while (nal_end - nal > max_fragment_payload_size) {
            session->video_buffer_ptr = session->video_buffer + sizeof(rtp_header_t);
            *(session->video_buffer_ptr++) = nri | 28;             // Fragmented Unit Indicator (FU-A)
            *(session->video_buffer_ptr++) = (first_fragment ? 0x80 : 0x00) | type;
            memcpy(session->video_buffer_ptr, nal, max_fragment_payload_size);
            session->video_buffer_ptr += max_fragment_payload_size;

            r = session_send_data(session, false);
            if (r)
                return -1;

            nal += max_fragment_payload_size;
            first_fragment = false;
        }

        session->video_buffer_ptr = session->video_buffer + sizeof(rtp_header_t);
        *(session->video_buffer_ptr++) = nri | 28;             // Fragmented Unit Indicator (FU-A)
        *(session->video_buffer_ptr++) = 0x40 | type;          // ending fragment flag
        memcpy(session->video_buffer_ptr, nal, nal_end - nal);
        session->video_buffer_ptr += (nal_end - nal);

        r = session_send_data(session, last);
        if (r)
            return -1;
    }

    return 0;
}


const static JOCTET EOI_BUFFER[1] = { JPEG_EOI };

typedef struct {
  struct jpeg_source_mgr pub;
  const JOCTET *data;
  size_t len;
} jpeg_memory_src_mgr;


static void jpeg_memory_src_init_source(j_decompress_ptr cinfo) {
}


static boolean jpeg_memory_src_fill_input_buffer(j_decompress_ptr cinfo) {
  jpeg_memory_src_mgr *src = (jpeg_memory_src_mgr *)cinfo->src;
  // No more data.  Probably an incomplete image;  just output EOI.
  src->pub.next_input_byte = EOI_BUFFER;
  src->pub.bytes_in_buffer = 1;
  return TRUE;
}


static void jpeg_memory_src_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
  jpeg_memory_src_mgr *src = (jpeg_memory_src_mgr *)cinfo->src;
  if (src->pub.bytes_in_buffer < num_bytes) {
    // Skipping over all of remaining data;  output EOI.
    src->pub.next_input_byte = EOI_BUFFER;
    src->pub.bytes_in_buffer = 1;
  } else {
    // Skipping over only some of the remaining data.
    src->pub.next_input_byte += num_bytes;
    src->pub.bytes_in_buffer -= num_bytes;
  }
}


static void jpeg_memory_src_term_source(j_decompress_ptr cinfo) {
}


static void jpeg_memory_src_set_source_mgr(j_decompress_ptr cinfo, const char* data, size_t len) {
  if (cinfo->src == 0) {
    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)
      ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(jpeg_memory_src_mgr));
  }

  jpeg_memory_src_mgr *src = (jpeg_memory_src_mgr *)cinfo->src;
  src->pub.init_source = jpeg_memory_src_init_source;
  src->pub.fill_input_buffer = jpeg_memory_src_fill_input_buffer;
  src->pub.skip_input_data = jpeg_memory_src_skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; // default
  src->pub.term_source = jpeg_memory_src_term_source;
  // fill the buffers
  src->data = (const JOCTET *)data;
  src->len = len;
  src->pub.bytes_in_buffer = len;
  src->pub.next_input_byte = src->data;
}


int grab_camera_frame(x264_picture_t *pic) {
#if 0
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGD(TAG, "Camera capture failed");
        return -1;
    }

    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr jerr;

    dinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&dinfo);
    jpeg_memory_src_set_source_mgr(&dinfo, (const char *)fb->buf, fb->len);

    jpeg_read_header(&dinfo, TRUE);

    dinfo.raw_data_out = true;
    dinfo.out_color_space = JCS_YCbCr;
    dinfo.scale_num = 1;
    dinfo.scale_denom = 8;
    jpeg_start_decompress(&dinfo);

    ESP_LOGI(TAG, "JPEG output size: %dx%d", dinfo.output_width, dinfo.output_height);

    #define SAMPLE_SIZE 16

    uint8_t *data_y[SAMPLE_SIZE],
            *data_cb[SAMPLE_SIZE/2],
            *data_cr[SAMPLE_SIZE/2];
    uint8_t **data[3];
    data[0] = data_y;
    data[1] = data_cb;
    data[2] = data_cr;

    for (size_t j=0; j<dinfo.output_height; ) {
        for (size_t i=0; i<SAMPLE_SIZE; i+=2) {
            data_y[i]   = pic->img.plane[0] + dinfo.output_width * (i+j);
            data_y[i+1] = pic->img.plane[0] + dinfo.output_width * (i+1+j);
            data_cb[i / 2] = pic->img.plane[1] + dinfo.output_width / 2 * ((i + j) / 2);
            data_cr[i / 2] = pic->img.plane[2] + dinfo.output_width / 2 * ((i + j) / 2);
        }

        j += jpeg_read_raw_data(&dinfo, data, SAMPLE_SIZE);
    }

    ESP_LOGI(TAG, "JPEG finishing");

    jpeg_finish_decompress(&dinfo);

    ESP_LOGI(TAG, "JPEG destroying");
    jpeg_destroy_decompress(&dinfo);

    ESP_LOGI(TAG, "Returning frame buffer");
    esp_camera_fb_return(fb);

    ESP_LOGI(TAG, "Done grabbing a frame");
#endif
    return 0;
}


static streaming_session_t *streaming_sessions = NULL;
#if 0
static SemaphoreHandle_t streaming_sessions_mutex = NULL;
#endif

#if 0
TaskHandle_t stream_task_handle = NULL;
EventGroupHandle_t stream_control_events;
#endif

#define STREAM_CONTROL_EVENT_STOP (1 << 0)


void stream_task(void *arg);


int streaming_init() {
    streaming_sessions = NULL;
#if 0
    streaming_sessions_mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(streaming_sessions_mutex);
#endif

#if 0
    stream_control_events = xEventGroupCreate();
    if (stream_control_events == NULL) {
        ESP_LOGE(TAG, "Failed to create camera control event group");
        return -1;
    }
#endif

    return 0;
}


static bool stream_running() {
#if 0
    return stream_task_handle != NULL;
#endif
}

static void stream_start() {
    if (stream_running())
        return;

#if 0
    xEventGroupClearBits(stream_control_events, STREAM_CONTROL_EVENT_STOP);
#endif

#if 0
    xTaskCreate(stream_task, "Camera Stream",
                4096*8, NULL, 1, &stream_task_handle);
#else
    pthread_t tid;
    pthread_create(&tid, NULL, stream_task, NULL);
#endif
}

static void stream_stop() {
    if (!stream_running())
        return;

    ESP_LOGI(TAG, "Stopping video stream");
#if 0
    xEventGroupSetBits(stream_control_events, STREAM_CONTROL_EVENT_STOP);
#endif
}


static void streaming_sessions_lock() {
#if 0
    xSemaphoreTake(streaming_sessions_mutex, 5000 / portTICK_PERIOD_MS);
#endif
}


static void streaming_sessions_unlock() {
#if 0
    xSemaphoreGive(streaming_sessions_mutex);
#endif
}


static streaming_session_t *streaming_session_new(camera_session_t *settings) {
    streaming_session_t *session = (streaming_session_t*) calloc(1, sizeof(streaming_session_t));

    session->settings = settings;
    session->started = false;
    session->failed = false;

    session->timestamp = 0;

    session->sequence = 123;
    session->video_buffer = (uint8_t*) calloc(1, 2048);
    session->video_buffer_ptr = session->video_buffer + sizeof(rtp_header_t);

    session_init_crypto(session);

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(session->settings->controller_video_port);
    if (inet_pton(AF_INET, session->settings->controller_ip_address, &remote_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Failed to parse controller IP address");
        return NULL;
    }

    if (connect(session->settings->video_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to set video socket connect destination");
        return NULL;
    }

    return session;
}


void streaming_session_free(streaming_session_t *session) {
    if (session->video_buffer)
        free(session->video_buffer);

    free(session);
}


int streaming_sessions_add(camera_session_t *settings) {
    streaming_sessions_lock();

    streaming_session_t *session = streaming_session_new(settings);
    if (!session) {
        ESP_LOGI(TAG, "Failed to create streaming session");
        streaming_sessions_unlock();
        return -1;
    }

    ESP_LOGI(TAG, "Adding streaming session");

    session->next = streaming_sessions;
    streaming_sessions = session;

    stream_start();

    streaming_sessions_unlock();

    return 0;
}


void streaming_sessions_remove(camera_session_t *settings) {
    streaming_sessions_lock();

    ESP_LOGI(TAG, "Removing streaming session");

    if (!streaming_sessions) {
        streaming_sessions_unlock();
        return;
    }

    if (streaming_sessions->settings == settings) {
        streaming_session_t *s = streaming_sessions;
        streaming_sessions = streaming_sessions->next;
        streaming_session_free(s);
    } else {
        streaming_session_t *t = streaming_sessions;
        while (t->next) {
            if (t->next->settings == settings) {
                streaming_session_t *s = t->next;
                t->next = t->next->next;
                streaming_session_free(s);
                break;
            }
            t = t->next;
        }
    }

    if (!streaming_sessions)
        stream_stop();

    streaming_sessions_unlock();
}


void stream_task(void *arg) {
    ESP_LOGI(TAG, "Starting streaming");
#if 0
    ESP_LOGI(TAG, "Total free memory: %u", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
#endif

    x264_param_t param;
    x264_param_default(&param);

    if (x264_param_default_preset(&param, "ultrafast", "zerolatency") < 0) {
        ESP_LOGE(TAG, "Failed to initialize H.264 preset");
        return;
    }

    param.i_width = VIDEO_WIDTH;
    param.i_height = VIDEO_HEIGHT;
#if 0
    param.i_bitdepth = 8;
#endif
    param.i_csp = X264_CSP_I420;
    // param.i_fps_num = 1;
    // param.i_fps_den = 1;
    param.i_threads = 1;
    param.b_repeat_headers = 1;

    // param.i_frame_reference = 1;
    // param.i_bframe = 0;

    param.i_keyint_max = 10;
    // param.b_intra_refresh = 0;

    if (x264_param_apply_profile(&param, "baseline") < 0) {
        ESP_LOGE(TAG, "Failed to intialize H.264 profile");
        return;
    }

    x264_picture_t pic, pic_out;

    x264_picture_init(&pic);
    pic.img.i_csp = param.i_csp;
    pic.img.i_plane = 3;
    pic.img.i_stride[0] = param.i_width;
    pic.img.i_stride[1] = param.i_width / 2;
    pic.img.i_stride[2] = param.i_width / 2;

    const int plane_size = VIDEO_WIDTH * VIDEO_HEIGHT;
    pic.img.plane[0] = (uint8_t*) malloc(plane_size * 3);
    pic.img.plane[1] = pic.img.plane[0] + plane_size;
    pic.img.plane[2] = pic.img.plane[1] + plane_size / 4;

    x264_t *encoder = x264_encoder_open(&param);
    if (!encoder) {
        ESP_LOGE(TAG, "Failed to open H.264 encoder");
        free(pic.img.plane[0]);
        return;
    }

    x264_nal_t *nal = NULL;
    int i_nal;
    int frame_size = 0;

    ESP_LOGI(TAG, "Executing streaming loop");
#if 0
    ESP_LOGI(TAG, "Total free memory: %u", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
#endif

    while (true) {
#if 0
        if (xEventGroupGetBits(stream_control_events) & STREAM_CONTROL_EVENT_STOP)
            break;
#endif

        ESP_LOGI(TAG, "Getting a frame");
#if 0
        ESP_LOGI(TAG, "Total free memory: %u", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
#endif

        if (grab_camera_frame(&pic) < 0) {
            continue;
        }

        ESP_LOGI(TAG, "Encoding a frame");
#if 0
        ESP_LOGI(TAG, "Total free memory: %u", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
#endif

        frame_size = x264_encoder_encode(encoder, &nal, &i_nal, &pic, &pic_out);
        // TODO: free up pic_out

        ESP_LOGI(TAG, "Encoded frame, size = %d", frame_size);
        if( frame_size < 0 ) {
            ESP_LOGE(TAG, "Image H264 encoding failed error = %d", frame_size);
            continue;
        } else if( frame_size == 0 ) {
            ESP_LOGE(TAG, "Frame is empty");
            continue;
        }

        // dump_binary("Frame: ", nal->p_payload, 20);

        streaming_sessions_lock();
        for (streaming_session_t *session = streaming_sessions; session; session=session->next) {
            if (session->started)
                continue;

            session->started = true;
            session->timestamp = get_time_millis() * 1000;
            if (send_rtcp_sender_report(session)) {
                ESP_LOGE(TAG, "Error sending RTCP SenderReport to %s:%d",
                         session->settings->controller_ip_address,
                         session->settings->controller_video_port);
                session->failed = true;
            }
        }

        for (streaming_session_t *session = streaming_sessions; session; session=session->next) {
            session->timestamp = get_time_millis() * 1000;
        }
        ESP_LOGI(TAG, "Sending frame data");

        // dump_binary("Frame: ", nal->p_payload, frame_size);

        uint8_t* end = nal->p_payload + frame_size;
        uint8_t* nal_data = find_nal_start(nal->p_payload, end);

        while (nal_data < end) {
            nal_data += (nal_data[2] == 1) ? 3 : 4;  // skip header
            uint8_t* next_nal_data = find_nal_start(nal_data, end);

            size_t nal_data_size = next_nal_data - nal_data;

            for (streaming_session_t *session = streaming_sessions; session; session=session->next) {
                if (session_send_nal(session, nal_data, nal_data_size, (next_nal_data == end)))
                    session->failed = true;
            }

            nal_data = next_nal_data;
        }

        for (streaming_session_t *session = streaming_sessions; session; session=session->next) {
            if (session_flush_buffered(session, true))
                session->failed = true;
        }

        // cleanup failed streaming sessions
        while (streaming_sessions && streaming_sessions->failed) {
            streaming_session_t *t = streaming_sessions;
            streaming_sessions = streaming_sessions->next;
            streaming_session_free(t);
        }
        if (!streaming_sessions) {
            streaming_sessions_unlock();
            break;
        }

        streaming_session_t *t = streaming_sessions;
        while (t->next) {
            if (t->next->failed) {
                streaming_session_t *tt = t;
                t = t->next;
                streaming_session_free(tt);
            } else {
                t = t->next;
            }
        }

        streaming_sessions_unlock();

        ESP_LOGI(TAG, "Done sending frame data");

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    x264_encoder_close(encoder);

    free(pic.img.plane[0]);

    ESP_LOGI(TAG, "Done with stream");

#if 0
    stream_task_handle = NULL;
    vTaskDelete(NULL);
#endif
}
