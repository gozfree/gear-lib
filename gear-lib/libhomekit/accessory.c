#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#if 0
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_camera.h>
#include "camera.h"
#endif

#if 0
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif
#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include "camera_session.h"
#include "crypto.h"
#include "streaming.h"

#define ESP_LOGI(tag, format, ... ) printf(format, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ... ) printf(format, ##__VA_ARGS__)

#define vTaskDelay sleep

#define CAMERA_FRAME_RATE 30
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480

// #define VIDEO_WIDTH 160
// #define VIDEO_HEIGHT 120
#define VIDEO_WIDTH 80
#define VIDEO_HEIGHT 60
#define STREAMING_STATUS_AVAILABLE 0
#define STREAMING_STATUS_IN_USE 1
#define STREAMING_STATUS_UNAVAILABLE 2

#define portTICK_PERIOD_MS  1

#if 0
static ip4_addr_t ip_address;


void camera_accessory_set_ip_address(ip4_addr_t ip) {
    ip_address = ip;
}
#endif


typedef enum {
    SESSION_COMMAND_END = 0,
    SESSION_COMMAND_START = 1,
    SESSION_COMMAND_SUSPEND = 2,
    SESSION_COMMAND_RESUME = 3,
    SESSION_COMMAND_RECONFIGURE = 4,
} session_command_t;


#if 0
const int camera_led_gpio = CONFIG_CAMERA_PIN_LED;
bool camera_led_on = false;
#endif

void camera_led_set(bool on) {
#if 0
    gpio_set_level(camera_led_gpio, on ? 1 : 0);
    camera_led_on = on;
#endif
}

void camera_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            camera_led_set(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            camera_led_set(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    camera_led_set(true);

#if 0
    vTaskDelete(NULL);
#endif
}

void camera_identify(homekit_value_t _value) {
    printf("Camera identify\n");
#if 0
    xTaskCreate(camera_identify_task, "Camera identify", 512, NULL, 2, NULL);
#else
    pthread_t tid;
    pthread_create(&tid, NULL, camera_identify_task, NULL);
#endif
}


homekit_value_t camera_streaming_status_get() {
    tlv_values_t *tlv = tlv_new();
    tlv_add_integer_value(tlv, 1, 1, STREAMING_STATUS_AVAILABLE);
    return HOMEKIT_TLV(tlv);
}


static camera_session_t *camera_sessions;


int camera_sessions_add(camera_session_t *session) {
    if (!camera_sessions) {
        camera_sessions = session;
    } else {
        camera_session_t *t = camera_sessions;
        int i = 1;
        while (t->next) {
            i++;
            t = t->next;
        }
        if (i >= MAX_CAMERA_SESSIONS) {
            return -1;
        }

        t->next = session;
        session->next = NULL;
    }

    return 0;
}


void camera_sessions_remove(camera_session_t *session) {
    if (!camera_sessions) {
        return;
    }

    if (camera_sessions == session) {
        camera_sessions = camera_sessions->next;
    } else {
        camera_session_t *t = camera_sessions;
        while (t->next) {
            if (t->next == session) {
                t->next = t->next->next;
                break;
            }
            t = t->next;
        }
    }
}


camera_session_t *camera_sessions_find_by_client_id(homekit_client_id_t client_id) {
    camera_session_t *session = camera_sessions;
    while (session) {
        if (session->client_id == client_id) {
            break;
        }
        session = session->next;
    }

    return session;
}


homekit_value_t camera_setup_endpoints_get() {
    ESP_LOGI(TAG, "Creating setup endpoints response");
    homekit_client_id_t client_id = homekit_get_client_id();
    if (!client_id) {
        ESP_LOGI(TAG, "No client found");
        return HOMEKIT_TLV(tlv_new());
    }

    camera_session_t *session = camera_sessions_find_by_client_id(client_id);
    if (!session) {
        return HOMEKIT_TLV(tlv_new());
    }

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    tlv_values_t *accessory_address = tlv_new();
    tlv_add_integer_value(accessory_address, 1, 1, 0);
#if 0
    tlv_add_string_value(accessory_address, 2, ip4addr_ntoa(&ip_address));
#endif

    getsockname(session->video_socket, (struct sockaddr*)&addr, &addr_len);
    tlv_add_integer_value(accessory_address, 3, 2, addr.sin_port);

    // getsockname(session->audio_socket, (struct sockaddr*)&addr, &addr_len);
    // tlv_add_integer_value(accessory_address, 4, 2, addr.sin_port);
    tlv_add_integer_value(accessory_address, 4, 2, addr.sin_port+1);

    tlv_values_t *video_rtp_params = tlv_new();
    tlv_add_integer_value(video_rtp_params, 1, 1, session->srtp_video_crypto_suite);
    tlv_add_value(video_rtp_params, 2, (unsigned char*)session->srtp_video_master_key, session->srtp_video_master_key_size);
    tlv_add_value(video_rtp_params, 3, (unsigned char*)session->srtp_video_master_salt, session->srtp_video_master_salt_size);

    tlv_values_t *audio_rtp_params = tlv_new();
    tlv_add_integer_value(audio_rtp_params, 1, 1, session->srtp_audio_crypto_suite);
    tlv_add_value(audio_rtp_params, 2, (unsigned char*)session->srtp_audio_master_key, session->srtp_audio_master_key_size);
    tlv_add_value(audio_rtp_params, 3, (unsigned char*)session->srtp_audio_master_salt, session->srtp_audio_master_salt_size);

    tlv_values_t *response = tlv_new();
    tlv_add_value(response, 1, (unsigned char*)session->session_id, 16);
    tlv_add_integer_value(response, 2, 1, session->status);
    tlv_add_tlv_value(response, 3, accessory_address);
    tlv_add_tlv_value(response, 4, video_rtp_params);
    tlv_add_tlv_value(response, 5, audio_rtp_params);
    // TODO: automatically determine size
    tlv_add_integer_value(response, 6, 1, session->video_ssrc);
    tlv_add_integer_value(response, 7, 1, session->audio_ssrc);

    tlv_free(accessory_address);
    tlv_free(video_rtp_params);
    tlv_free(audio_rtp_params);

    return HOMEKIT_TLV(response);
}


void camera_setup_endpoints_set(homekit_value_t value) {
    if (value.format != homekit_format_tlv) {
        ESP_LOGE(TAG, "Invalid value format: %d", value.format);
        return;
    }

    #define error_msg "Failed to setup endpoints: "

    homekit_client_id_t client_id = homekit_get_client_id();

    camera_session_t *session = camera_sessions_find_by_client_id(client_id);
    if (!session) {
        session = camera_session_new();
        session->client_id = client_id;
    }

    tlv_values_t *request = value.tlv_values;

    tlv_t *v;
    int x;

    v = tlv_get_value(request, 1);
    if (!v) {
        ESP_LOGE(TAG, error_msg "no session ID field");
        camera_session_free(session);
        return;
    }

    if (v->size != 16) {
        ESP_LOGE(TAG, error_msg "session ID field has invalid size (%d)", v->size);
        camera_session_free(session);
        return;
    }
    memcpy(session->session_id, (char*)v->value, v->size);
    session->session_id[v->size] = 0;

    tlv_values_t *controller_address = tlv_get_tlv_value(request, 3);
    if (!controller_address) {
        ESP_LOGE(TAG, error_msg "no controller address field");
        camera_session_free(session);
        return;
    }

    x = tlv_get_integer_value(controller_address, 1, -1);
    if (x == -1) {
        ESP_LOGE(TAG, error_msg "no controller IP address version field");
        tlv_free(controller_address);
        camera_session_free(session);
        return;
    }
    session->controller_ip_version = x;

    v = tlv_get_value(controller_address, 2);
    if (!v) {
        ESP_LOGE(TAG, error_msg "no controller IP address field");
        tlv_free(controller_address);
        camera_session_free(session);
        return;
    }
    char *ip_address = strndup((char*)v->value, v->size);
    session->controller_ip_address = ip_address;

    x = tlv_get_integer_value(controller_address, 3, -1);
    if (x == -1) {
        ESP_LOGE(TAG, error_msg "no controller video port field");
        tlv_free(controller_address);
        camera_session_free(session);
        return;
    }
    session->controller_video_port = x;

    x = tlv_get_integer_value(controller_address, 4, -1);
    if (x == -1) {
        ESP_LOGE(TAG, error_msg "no controller audio port field");
        tlv_free(controller_address);
        camera_session_free(session);
        return;
    }
    session->controller_audio_port = x;

    tlv_free(controller_address);

    tlv_values_t *rtp_params = tlv_get_tlv_value(request, 4);
    if (!rtp_params) {
        ESP_LOGE(TAG, error_msg "no video RTP params field");
        camera_session_free(session);
        return;
    }

    x = tlv_get_integer_value(rtp_params, 1, -1);
    if (x == -1) {
        ESP_LOGE(TAG, error_msg "no video RTP params crypto suite field");
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    session->srtp_video_crypto_suite = x;

    v = tlv_get_value(rtp_params, 2);
    if (!v) {
        ESP_LOGE(TAG, error_msg "no video RTP params master key field");
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    if (v->size >= sizeof(session->srtp_video_master_key)) {
        ESP_LOGE(TAG, error_msg "invalid video RTP params master key size (%d)", v->size);
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    memcpy(session->srtp_video_master_key, (char*)v->value, v->size);
    session->srtp_video_master_key_size = v->size;

    v = tlv_get_value(rtp_params, 3);
    if (!v) {
        ESP_LOGE(TAG, error_msg "no video RTP params master salt field");
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    if (v->size >= sizeof(session->srtp_video_master_salt)) {
        ESP_LOGE(TAG, error_msg "invalid video RTP params master key size (%d)", v->size);
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    memcpy(session->srtp_video_master_salt, (char*)v->value, v->size);
    session->srtp_video_master_salt_size = v->size;

    tlv_free(rtp_params);

    rtp_params = tlv_get_tlv_value(request, 5);
    if (!rtp_params) {
        ESP_LOGE(TAG, error_msg "no audio RTP params field");
        camera_session_free(session);
        return;
    }

    x = tlv_get_integer_value(rtp_params, 1, -1);
    if (x == -1) {
        ESP_LOGE(TAG, error_msg "no audio RTP params crypto suite field");
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    session->srtp_audio_crypto_suite = x;

    v = tlv_get_value(rtp_params, 2);
    if (!v) {
        ESP_LOGE(TAG, error_msg "no audio RTP params master key field");
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    if (v->size >= sizeof(session->srtp_audio_master_key)) {
        ESP_LOGE(TAG, error_msg "invalid audio RTP params master key size (%d)", v->size);
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    memcpy(session->srtp_audio_master_key, (char*)v->value, v->size);
    session->srtp_audio_master_key_size = v->size;

    v = tlv_get_value(rtp_params, 3);
    if (!v) {
        ESP_LOGE(TAG, error_msg "no audio RTP params master salt field");
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    if (v->size >= sizeof(session->srtp_audio_master_salt)) {
        ESP_LOGE(TAG, error_msg "invalid audio RTP params master salt size (%d)", v->size);
        tlv_free(rtp_params);
        camera_session_free(session);
        return;
    }
    memcpy(session->srtp_audio_master_salt, (char*)v->value, v->size);
    session->srtp_audio_master_salt_size = v->size;

    tlv_free(rtp_params);

    #undef error_msg

    session->video_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (session->video_socket < 0) {
        ESP_LOGE(TAG, "Failed to open video stream socket, error = %d", errno);
        session->status = 2;
        return;
    }

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(session->controller_video_port);
    if (inet_pton(AF_INET, session->controller_ip_address, &remote_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Failed to parse controller IP address");
        session->status = 2;
        return;
    }

    if (connect(session->video_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to set video socket connect destination");
        session->status = 2;
        return;
    }

    if (camera_sessions_add(session)) {
        // session registration failed
        session->status = 2;
        return;
    }
}

homekit_value_t camera_selected_rtp_configuration_get() {
    return HOMEKIT_TLV(tlv_new());
}

void camera_selected_rtp_configuration_set(homekit_value_t value) {
#if 0
    if (value.format != homekit_format_tlv) {
        ESP_LOGE(TAG, "Failed to setup selected RTP config: invalid value format: %d", value.format);
        return;
    }

    #define error_msg "Failed to setup selected RTP config: %s"

    camera_session_t *session = camera_sessions_find_by_client_id(homekit_get_client_id());
    if (!session)
        return;

    tlv_values_t *request = value.tlv_values;
    int x;

    tlv_values_t *session_control = tlv_get_tlv_value(request, 1);
    if (!session_control) {
        ESP_LOGE(TAG, error_msg, "no session control field");
        return;
    }
    tlv_t *session_id = tlv_get_value(session_control, 1);
    if (!session_id) {
        ESP_LOGE(TAG, error_msg, "no session ID field");
        tlv_free(session_control);
        return;
    }
    // TODO: find session with given ID
    if (strncmp(session->session_id, (char*)session_id->value, sizeof(session->session_id)-1)) {
        ESP_LOGE(TAG, error_msg, "invalid session ID");
        tlv_free(session_control);
        return;
    }

    int session_command = tlv_get_integer_value(session_control, 2, -1);
    if (session_command == -1) {
        ESP_LOGE(TAG, error_msg, "no command field");
        tlv_free(session_control);
        return;
    }
    tlv_free(session_control);

    if (session_command == SESSION_COMMAND_START || session_command == SESSION_COMMAND_RECONFIGURE) {
        tlv_values_t *selected_video_params = tlv_get_tlv_value(request, 2);
        if (!selected_video_params) {
            ESP_LOGE(TAG, error_msg, "no selected video params field");
            return;
        }

        tlv_values_t *video_rtp_params = tlv_get_tlv_value(selected_video_params, 4);
        if (!video_rtp_params) {
            ESP_LOGE(TAG, error_msg, "no selected video RTP params field");
            tlv_free(selected_video_params);
            return;
        }

        x = tlv_get_integer_value(video_rtp_params, 1, -1);
        if (x == -1) {
            ESP_LOGE(TAG, error_msg, "no selected video RTP payload type field");
            tlv_free(video_rtp_params);
            tlv_free(selected_video_params);
            return;
        }
        session->video_rtp_payload_type = x;

        x = tlv_get_integer_value(video_rtp_params, 2, 0);
        if (x == 0) {
            ESP_LOGE(TAG, error_msg, "no selected video RTP SSRC field");
            tlv_free(video_rtp_params);
            tlv_free(selected_video_params);
            return;
        }
        // session->video_ssrc = x;

        x = tlv_get_integer_value(video_rtp_params, 3, -1);
        if (x == -1) {
            ESP_LOGE(TAG, error_msg, "no selected video RTP max bitrate field");
            tlv_free(video_rtp_params);
            tlv_free(selected_video_params);
            return;
        }
        session->video_rtp_max_bitrate = x;

        // TODO: parse min RTCP interval

        x = tlv_get_integer_value(video_rtp_params, 5, -1);
        if (x == -1) {
            ESP_LOGE(TAG, error_msg, "no selected video RTP max MTU field");
            tlv_free(video_rtp_params);
            tlv_free(selected_video_params);
            return;
        }
        session->video_rtp_max_mtu = x;

        tlv_free(video_rtp_params);
        tlv_free(selected_video_params);
    }

    // TODO: process command
    ESP_LOGI(TAG, "Processing session command %d", session_command);
    switch (session_command) {
        case SESSION_COMMAND_START:
        case SESSION_COMMAND_RESUME:
            streaming_sessions_add(session);
            break;

        case SESSION_COMMAND_RECONFIGURE:
            // streaming_sessions_add(session);

            break;
        case SESSION_COMMAND_SUSPEND:
            streaming_sessions_remove(session);
            break;

        case SESSION_COMMAND_END: {
            streaming_sessions_remove(session);
            camera_sessions_remove(session);

            break;
        }
    }
#endif

    #undef error_msg
}

tlv_values_t supported_video_config = {};
tlv_values_t supported_audio_config = {};
tlv_values_t supported_rtp_config = {};


void camera_on_resource(const char *body, size_t body_size) {
    ESP_LOGI(TAG, "Resource payload: %s", body);
    // cJSON *json = cJSON_Parse(body);
    // process json
    // cJSON_Delete(json);

#if 0
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGD(TAG, "Camera capture failed");
        static unsigned char error_payload[] = "HTTP/1.1 500 Camera capture error\r\n\r\n";
        homekit_client_send(error_payload, sizeof(error_payload)-1);
        return;
    }

    static unsigned char success_payload[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Disposition: inline; filename=capture.jpg\r\n"
        "Content-Length: ";

    homekit_client_send(success_payload, sizeof(success_payload)-1);

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d\r\n\r\n", fb->len);

    homekit_client_send((unsigned char*) buffer, strlen(buffer));
    homekit_client_send(fb->buf, fb->len);

    esp_camera_fb_return(fb);
#endif
}


void camera_on_event(homekit_event_t event) {
    if (event == HOMEKIT_EVENT_CLIENT_DISCONNECTED) {
        camera_session_t *session = camera_sessions_find_by_client_id(homekit_get_client_id());
        if (session) {
            streaming_sessions_remove(session);
            camera_sessions_remove(session);
            camera_session_free(session);
        }
    }
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1,
                      .category=homekit_accessory_category_ip_camera,
                      .config_number=3,
                      .services=(homekit_service_t*[])
    {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Camera"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1"),
            HOMEKIT_CHARACTERISTIC(MODEL, "1"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, camera_identify),
            NULL
        }),
        HOMEKIT_SERVICE(CAMERA_RTP_STREAM_MANAGEMENT, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(
                SUPPORTED_VIDEO_STREAM_CONFIGURATION,
                .value=HOMEKIT_TLV_(&supported_video_config)
            ),
            HOMEKIT_CHARACTERISTIC(
                SUPPORTED_AUDIO_STREAM_CONFIGURATION,
                .value=HOMEKIT_TLV_(&supported_audio_config)
            ),
            HOMEKIT_CHARACTERISTIC(
                SUPPORTED_RTP_CONFIGURATION,
                .value=HOMEKIT_TLV_(&supported_rtp_config)
            ),
            HOMEKIT_CHARACTERISTIC(STREAMING_STATUS, .getter=camera_streaming_status_get),
            HOMEKIT_CHARACTERISTIC(
                SETUP_ENDPOINTS,
                .getter=camera_setup_endpoints_get,
                .setter=camera_setup_endpoints_set
            ),
            HOMEKIT_CHARACTERISTIC(
                SELECTED_RTP_STREAM_CONFIGURATION,
                .getter=camera_selected_rtp_configuration_get,
                .setter=camera_selected_rtp_configuration_set
            ),
            NULL
        }),
        HOMEKIT_SERVICE(MICROPHONE, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(VOLUME, 0),
            HOMEKIT_CHARACTERISTIC(MUTE, false),
            NULL
        }),
        NULL
    }),
    NULL
};
#pragma GCC diagnostic pop


homekit_server_config_t config = {
    .accessories = accessories,
//    .password = "111-11-111",
    .password = "123-45-678",
    .setupId="1QJ8",
    .on_event = camera_on_event,
    .on_resource = camera_on_resource,
};


void camera_accessory_init() {
#if 0
    ESP_LOGI(TAG, "Free heap: %d", xPortGetFreeHeapSize());

    // ESP_ERROR_CHECK(gpio_install_isr_service(0));
    gpio_set_direction(camera_led_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(camera_led_gpio, 1);

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("camera", ESP_LOG_VERBOSE);

    /* IO13, IO14 is designed for JTAG by default,
     * to use it as generalized input,
     * firstly declair it as pullup input */
    gpio_config_t conf;
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_up_en = GPIO_PULLUP_ENABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pin_bit_mask = 1LL << 13;
    gpio_config(&conf);
    conf.pin_bit_mask = 1LL << 14;
    gpio_config(&conf);

    camera_config_t camera_config = {
        .ledc_channel = LEDC_CHANNEL_0,
        .ledc_timer = LEDC_TIMER_0,
        .pin_d0 = CAMERA_PIN_D0,
        .pin_d1 = CAMERA_PIN_D1,
        .pin_d2 = CAMERA_PIN_D2,
        .pin_d3 = CAMERA_PIN_D3,
        .pin_d4 = CAMERA_PIN_D4,
        .pin_d5 = CAMERA_PIN_D5,
        .pin_d6 = CAMERA_PIN_D6,
        .pin_d7 = CAMERA_PIN_D7,
        .pin_xclk = CAMERA_PIN_XCLK,
        .pin_pclk = CAMERA_PIN_PCLK,
        .pin_vsync = CAMERA_PIN_VSYNC,
        .pin_href = CAMERA_PIN_HREF,
        .pin_sscb_sda = CAMERA_PIN_SIOD,
        .pin_sscb_scl = CAMERA_PIN_SIOC,
        .pin_pwdn = CAMERA_PIN_PWDN,
        .pin_reset = CAMERA_PIN_RESET,
        .xclk_freq_hz = CONFIG_XCLK_FREQ,

        .frame_size = CAMERA_FRAME_SIZE,
        .pixel_format = PIXFORMAT_JPEG,
        .jpeg_quality = 15,

        .fb_count = 2,
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
#endif

    tlv_values_t *video_codec_params = tlv_new();
    tlv_add_integer_value(video_codec_params, 1, 1, 1);  // Profile ID
    tlv_add_integer_value(video_codec_params, 2, 1, 0);  // Level
    tlv_add_integer_value(video_codec_params, 3, 1, 0);  // Packetization mode

    tlv_values_t *video_attributes = tlv_new();
    // FIXME:
    // tlv_add_integer_value(video_attributes, 1, 2, CAMERA_WIDTH);  // Image width
    // tlv_add_integer_value(video_attributes, 2, 2, CAMERA_HEIGHT);  // Image height
    tlv_add_integer_value(video_attributes, 1, 2, 640);  // Image width
    tlv_add_integer_value(video_attributes, 2, 2, 480);  // Image height
    tlv_add_integer_value(video_attributes, 3, 1, CAMERA_FRAME_RATE);  // Frame rate

    tlv_values_t *video_codec_config = tlv_new();
    tlv_add_integer_value(video_codec_config, 1, 1, 0);  // Video codec type
    tlv_add_tlv_value(video_codec_config, 2, video_codec_params);  // Video codec params
    tlv_add_tlv_value(video_codec_config, 3, video_attributes);  // Video attributes

    tlv_add_tlv_value(&supported_video_config, 1, video_codec_config);

    tlv_free(video_codec_config);
    tlv_free(video_attributes);
    tlv_free(video_codec_params);

    tlv_values_t *audio_codec_params = tlv_new();
    tlv_add_integer_value(audio_codec_params, 1, 1, 1);  // Number of audio channels
    tlv_add_integer_value(audio_codec_params, 2, 1, 0);  // Bit-rate
    tlv_add_integer_value(audio_codec_params, 3, 1, 2);  // Sample rate

    tlv_values_t *audio_codec = tlv_new();
    tlv_add_integer_value(audio_codec, 1, 2, 3);
    tlv_add_tlv_value(audio_codec, 2, audio_codec_params);

    tlv_add_tlv_value(&supported_audio_config, 1, audio_codec);
    tlv_add_integer_value(&supported_audio_config, 2, 1, 0);  // Comfort noise support

    tlv_free(audio_codec);
    tlv_free(audio_codec_params);

    tlv_add_integer_value(&supported_rtp_config, 2, 1, 0);  // SRTP crypto suite

    homekit_server_init(&config);
    if (streaming_init()) {
        // TODO: panic
    }
}
