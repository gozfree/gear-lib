#include "libavcap.h"
#include <libthread.h>
#include <liblog.h>
#include <esp_camera.h>
#include <esp_log.h>

#define CONFIG_CAMERA_MODEL_ESP_EYE 1

#if CONFIG_CAMERA_MODEL_ESP_EYE
#define CAMERA_PIN_PWDN    -1
#define CAMERA_PIN_RESET   -1
#define CAMERA_PIN_SIOD    18
#define CAMERA_PIN_SIOC    23
#define CAMERA_PIN_XCLK    4

#define CAMERA_PIN_D0      34
#define CAMERA_PIN_D1      13
#define CAMERA_PIN_D2      14
#define CAMERA_PIN_D3      35
#define CAMERA_PIN_D4      39
#define CAMERA_PIN_D5      38
#define CAMERA_PIN_D6      37
#define CAMERA_PIN_D7      36
#define CAMERA_PIN_VSYNC   5
#define CAMERA_PIN_HREF    27
#define CAMERA_PIN_PCLK    25
#endif

#define CAMERA_FRAME_SIZE FRAMESIZE_VGA
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480

struct esp32_cam_ctx {
    camera_config_t conf;
    camera_fb_t *fb;
    struct thread *thread;
    bool is_streaming;
};

static camera_config_t default_camera_config = {
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

static void *cam_open(struct avcap_ctx *avcap, const char *dev, struct avcap_config *avconf)
{
    struct esp32_cam_ctx *c = calloc(1, sizeof(struct esp32_cam_ctx));
    if (!c) {
        printf("malloc esp32_cam_ctx failed!\n");
        return NULL;
    }
    memcpy(&c->conf, &default_camera_config, sizeof(camera_config_t));

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

    esp_err_t err = esp_camera_init(&c->conf);
    if (err != ESP_OK) {
        printf("Camera init failed with error 0x%x", err);
        return NULL;
    }

    memcpy(&avcap->conf, avconf, sizeof(struct avcap_config));
    return c;
}

static void cam_close(struct avcap_ctx *avcap)
{
    struct esp32_cam_ctx *c = (struct esp32_cam_ctx *)avcap->opaque;
    if (esp_camera_deinit() != ESP_OK) {
        printf("esp_camera_deinit failed!\n");
    }
    free(c);
}

static void *cam_thread(struct thread *t, void *arg)
{
    struct avcap_ctx *avcap = arg;
    struct esp32_cam_ctx *c = (struct esp32_cam_ctx *)avcap->opaque;
    struct media_frame media;
    struct video_frame *frame = &media.video;
    media.type = MEDIA_TYPE_VIDEO;

    video_frame_init(frame, avcap->conf.video.format, avcap->conf.video.width,
                    avcap->conf.video.height, MEDIA_MEM_DEEP);
    c->is_streaming = true;
    while (c->is_streaming) {
        c->fb = esp_camera_fb_get();
        if (!c->fb) {
            printf("Camera capture failed");
            return NULL;
        }
        printf("Camera capture width=%d, height=%d size=%d\n",
                        c->fb->width, c->fb->height, c->fb->len);
        avcap->on_media_frame(avcap, &media);
        esp_camera_fb_return(c->fb);
    }
    return NULL;
}

static int cam_start_stream(struct avcap_ctx *avcap)
{
    struct esp32_cam_ctx *c = (struct esp32_cam_ctx *)avcap->opaque;

    if (avcap->on_media_frame) {
        c->thread = thread_create(cam_thread, avcap);
        if (!c->thread) {
            printf("thread_create failed!\n");
            return -1;
        }
    }

    return 0;
}

static int cam_stop_stream(struct avcap_ctx *avcap)
{
    struct esp32_cam_ctx *c = (struct esp32_cam_ctx *)avcap->opaque;
    if (!c->is_streaming) {
        printf("esp32 stream stopped already!\n");
        return -1;
    }

    if (avcap->on_media_frame) {
        c->is_streaming = false;
        thread_join(c->thread);
        thread_destroy(c->thread);
    }
    return 0;
}

struct avcap_ops esp32cam_ops = {
    ._open        = cam_open,
    ._close       = cam_close,
    .ioctl        = NULL,
    .start_stream = cam_start_stream,
    .stop_stream  = cam_stop_stream,
};
