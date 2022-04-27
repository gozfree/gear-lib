#include <libposix.h>
#include <liblog.h>
#include <libmedia-io.h>
#include <libavcap.h>
#include <libfile.h>
//#include <libdarray.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUTPUT_V4L2     "v4l2_640_480.jpg"

static struct file *fp;
#if 0
static void *thread(void *arg)
{
    logi("thread\n");
    return NULL;
}
#endif

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
#if 1
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
#endif

void app_main(void)
{
    logi("%s:%d hello world!\n", __func__, __LINE__);
    cam_test();
}
