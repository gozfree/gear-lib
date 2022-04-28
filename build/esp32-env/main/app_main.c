#include <libposix.h>
#include <liblog.h>
#include <libmedia-io.h>
#include <libavcap.h>
#include <libfile.h>
#include <libhal.h>
//#include <libdarray.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_event_legacy.h>
#include <driver/gpio.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OUTPUT_V4L2     "v4l2_640_480.jpg"

#define GPIO_LED_RED    21
#define GPIO_LED_WHITE  22
#define GPIO_BUTTON     15
#define GPIO_BOOT       0

#define TAG "esp32"
static struct file *fp;

static int on_video(struct avcap_ctx *c, struct video_frame *video)
{
    static uint64_t last_ms = 0;
    static int luma = 0;
    static int i = 0;

    logi("video_frame[%" PRIu64 "] size=%" PRIu64 ", ts=%" PRIu64 " ms, gap=%" PRIu64 " ms\n",
          video->frame_id, video->total_size, video->timestamp/1000000, video->timestamp/1000000 - last_ms);
    last_ms = video->timestamp/1000000;
    luma = 2 * i++;
    luma *= i%2 ? 1: -1;
    //avcap_ioctl(c, VIDCAP_SET_LUMA, luma);
    file_write(fp, video->data[0], video->total_size);
    return 0;
}

static int on_frame(struct avcap_ctx *c, struct media_frame *frm)
{
    int ret;
    if (!c || !frm) {
        printf("on_frame invalid!\n");
        return -1;
    }
    switch (frm->type) {
    case MEDIA_TYPE_VIDEO:
        ret = on_video(c, &frm->video);
        break;
    default:
        printf("unsupport frame format");
        ret = -1;
        break;
    }
    return ret;
}

static void cam_test()
{
    struct avcap_config conf = {
        .type = AVCAP_TYPE_VIDEO,
        .backend = AVCAP_BACKEND_ESP32CAM,
        .video = {
            PIXEL_FORMAT_JPEG,
            640,
            480,
            .fps = {30, 1},
        }
    };
    struct avcap_ctx *avcap = avcap_open(NULL, &conf);
    if (!avcap) {
        printf("avcap_open uvc failed!\n");
        return;
    }

    fp = file_open(OUTPUT_V4L2, F_CREATE);

    avcap_start_stream(avcap, on_frame);
    sleep(5);
    avcap_stop_stream(avcap);
    file_close(fp);
    avcap_close(avcap);
}

static xQueueHandle gpio_evt_queue = NULL;
static int g_wifi_state = SYSTEM_EVENT_WIFI_READY;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void button_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

static void white_led_task(void *arg)
{
    while(1) {
        switch (g_wifi_state) {
        case SYSTEM_EVENT_STA_START:
        case SYSTEM_EVENT_STA_DISCONNECTED:
            gpio_set_level(GPIO_LED_WHITE, 0);
            usleep(300*1000);
            gpio_set_level(GPIO_LED_WHITE, 1);
            usleep(300*1000);
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
        case SYSTEM_EVENT_STA_GOT_IP:
            gpio_set_level(GPIO_LED_WHITE, 1);
            break;
        default:
            break;
        }
        usleep(10*1000);
    }
}

void gpio_button_init()
{
    gpio_config_t gpio_conf = {0};

    gpio_conf.pin_bit_mask = 1ULL << GPIO_BUTTON | 1ULL << GPIO_BOOT;
    gpio_conf.mode = GPIO_MODE_INPUT;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    ESP_ERROR_CHECK(gpio_config(&gpio_conf));

    //ESP_ERROR_CHECK(gpio_set_intr_type(GPIO_BUTTON, GPIO_INTR_ANYEDGE));
    //ESP_ERROR_CHECK(gpio_set_intr_type(GPIO_BOOT, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    gpio_isr_handler_add(GPIO_BUTTON, gpio_isr_handler, (void *)GPIO_BUTTON);
    gpio_isr_handler_add(GPIO_BOOT, gpio_isr_handler, (void *)GPIO_BOOT);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}

void gpio_led_init()
{
    gpio_config_t gpio_conf;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.pin_bit_mask = 1LL << GPIO_LED_RED;
    gpio_config(&gpio_conf);
    gpio_conf.pin_bit_mask = 1LL << GPIO_LED_WHITE;
    gpio_config(&gpio_conf);
    xTaskCreate(white_led_task, "white_led_task", 2048, NULL, 10, NULL);
}

static void wifi_event(void *ctx)
{
    system_event_t *event = (system_event_t *)ctx;
    g_wifi_state = event->event_id;
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "STA start blink");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "STA GOT IP: %s",
                        ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "STA disconnected, retry...");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "STA connected");
        break;
    default:
        ESP_LOGI(TAG, "unknown system event %d", event->event_id);
        break;
    }
}

void app_main(void)
{
    gpio_button_init();
    gpio_led_init();
    wifi_setup(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD, wifi_event);
    if (0)
    cam_test();
}
