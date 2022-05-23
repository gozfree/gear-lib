/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libavcap.h"
#include <libfile.h>
#include <libmedia-io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


#define VIDEO_DEV       "/dev/video0"
#define VIDEO_WIDTH     640
#define VIDEO_HEIGHT    480
#define OUTPUT_V4L2     "v4l2_640_480_yuv422p.yuv"
#define OUTPUT_DSHOW    "dshow_640_480_yuv422p.yuv"
#define INPUT_DUMMY     OUTPUT_V4L2
#define OUTPUT_DUMMY    "dummy_640_480_yuv422p.yuv"
#define OUTPUT_UVC      "uvc_640_480_yuv422p.yuv"
#define OUTPUT_PA       "pulseaudio_48k_s16le.pcm"

static struct file *fp;

static int on_video(struct avcap_ctx *c, struct video_frame *video)
{
    static uint64_t last_ms = 0;
    static int luma = 0;
    static int i = 0;

    printf("video_frame[%" PRIu64 "] size=%" PRIu64 ", ts=%" PRIu64 " ms, gap=%" PRIu64 " ms\n",
          video->frame_id, video->total_size, video->timestamp/1000000, video->timestamp/1000000 - last_ms);
    last_ms = video->timestamp/1000000;
    luma = 2 * i++;
    luma *= i%2 ? 1: -1;
    avcap_ioctl(c, VIDCAP_SET_LUMA, luma);
    file_write(fp, video->data[0], video->total_size);
    return 0;
}

static int on_audio(struct avcap_ctx *c, struct audio_frame *audio)
{
    printf("audio_frame[%" PRIu64 "] cnt=%d size=%" PRIu64 ", ts=%" PRIu64 " ms\n",
           audio->frame_id, audio->frames, audio->total_size, audio->timestamp/1000000);
    file_write(fp, audio->data[0], audio->total_size);
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
    case MEDIA_TYPE_AUDIO:
        ret = on_audio(c, &frm->audio);
        break;
    default:
        printf("unsupport frame format");
        ret = -1;
        break;
    }
    return ret;
}

int v4l2_test()
{
#if defined (OS_LINUX)
    struct avcap_ctx *avcap;
    struct video_frame *frm;
    struct avcap_config conf = {
            .type = AVCAP_TYPE_VIDEO,
            .backend = AVCAP_BACKEND_V4L2,
            .video = {
                PIXEL_FORMAT_YUY2,
                VIDEO_WIDTH,
                VIDEO_HEIGHT,
                .fps = {30, 1},
            },
    };
    printf("======== v4l2_test enter\n");
    avcap = avcap_open(VIDEO_DEV, &conf);
    if (!avcap) {
        printf("avcap_open v4l2 failed!\n");
        return -1;
    }
    //avcap_ioctl(avcap, VIDCAP_SET_CONF, &conf);
    avcap_ioctl(avcap, VIDCAP_GET_CAP, NULL);
    frm = video_frame_create(avcap->conf.video.format, avcap->conf.video.width, avcap->conf.video.height, MEDIA_MEM_SHALLOW);
    if (!frm) {
        printf("video_frame_create failed!\n");
        avcap_close(avcap);
        return -1;
    }
    printf("avcap info: %s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, avcap->conf.video.width, avcap->conf.video.height,
        avcap->conf.video.fps.num, avcap->conf.video.fps.den, pixel_format_to_string(avcap->conf.video.format));
    fp = file_open(OUTPUT_V4L2, F_CREATE);
    avcap_start_stream(avcap, on_frame);

    sleep(5);
    avcap_stop_stream(avcap);
    file_close(fp);
    video_frame_destroy(frm);
    avcap_close(avcap);
    printf("write %s fininshed!\n", OUTPUT_V4L2);
    printf("======== v4l2_test leave\n");
#endif
    return 0;
}

int dummy_test()
{
#if defined (OS_LINUX)
    struct avcap_ctx *avcap;
    struct video_frame *frm;
    struct avcap_config conf = {
            .type = AVCAP_TYPE_VIDEO,
            .backend = AVCAP_BACKEND_DUMMY,
            .video = {
                PIXEL_FORMAT_YUY2,
                VIDEO_WIDTH,
                VIDEO_HEIGHT,
                .fps = {30, 1},
                INPUT_DUMMY,
            }
    };
    printf("======== dummy_test enter\n");
    avcap = avcap_open(INPUT_DUMMY, &conf);
    if (!avcap) {
        printf("avcap_open dummy failed!\n");
        return -1;
    }
    frm = video_frame_create(avcap->conf.video.format, avcap->conf.video.width, avcap->conf.video.height, MEDIA_MEM_SHALLOW);
    if (!frm) {
        printf("video_frame_create failed!\n");
        avcap_close(avcap);
        return -1;
    }
    printf("%s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, avcap->conf.video.width, avcap->conf.video.height,
        avcap->conf.video.fps.num, avcap->conf.video.fps.den, pixel_format_to_string(avcap->conf.video.format));
    fp = file_open(OUTPUT_DUMMY, F_CREATE);
    avcap_start_stream(avcap, on_frame);
    sleep(5);
    avcap_stop_stream(avcap);
    file_sync(fp);
    file_close(fp);
    video_frame_destroy(frm);
    avcap_close(avcap);
    printf("write %s fininshed!\n", OUTPUT_DUMMY);
    printf("======== dummy_test leave\n");
#endif
    return 0;
}

int uvc_test()
{
#if defined (OS_LINUX)
    struct video_frame *frm;
    struct avcap_config conf = {
            .type = AVCAP_TYPE_VIDEO,
            .backend = AVCAP_BACKEND_UVC,
            .video = {
                PIXEL_FORMAT_YUY2,
                VIDEO_WIDTH,
                VIDEO_HEIGHT,
                .fps = {30, 1},
            },
    };
    struct avcap_ctx *avcap = avcap_open(VIDEO_DEV, &conf);
    if (!avcap) {
        printf("avcap_open uvc failed!\n");
        return -1;
    }
    frm = video_frame_create(avcap->conf.video.format, avcap->conf.video.width, avcap->conf.video.height, MEDIA_MEM_SHALLOW);
    if (!frm) {
        printf("video_frame_create failed!\n");
        avcap_close(avcap);
        return -1;
    }
    printf("%s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, avcap->conf.video.width, avcap->conf.video.height,
        avcap->conf.video.fps.num, avcap->conf.video.fps.den, pixel_format_to_string(avcap->conf.video.format));
    fp = file_open(OUTPUT_UVC, F_CREATE);
    avcap_start_stream(avcap, on_frame);
    sleep(5);
    avcap_stop_stream(avcap);
    file_sync(fp);
    file_close(fp);
    video_frame_destroy(frm);
    avcap_close(avcap);
    printf("write %s fininshed!\n", OUTPUT_UVC);
#endif
    return 0;
}

int dshow_test()
{
	struct avcap_ctx *avcap;
    struct video_frame *frm;
    struct avcap_config conf;
    conf.type = AVCAP_TYPE_VIDEO;
    conf.backend = AVCAP_BACKEND_DSHOW;
    conf.video.format = PIXEL_FORMAT_YUY2;
    conf.video.width = VIDEO_WIDTH;
    conf.video.height = VIDEO_HEIGHT;
    conf.video.fps.num = 30;
    conf.video.fps.den = 1;

    printf("======== dshow_test enter\n");
    avcap = avcap_open(VIDEO_DEV, &conf);
    if (!avcap) {
        printf("avcap_open dshow failed!\n");
        return -1;
    }
    //avcap_ioctl(avcap, VIDCAP_SET_CONF, &conf);
    avcap_ioctl(avcap, VIDCAP_GET_CAP, NULL);
    frm = video_frame_create(avcap->conf.video.format, avcap->conf.video.width, avcap->conf.video.height, MEDIA_MEM_SHALLOW);
    if (!frm) {
        printf("video_frame_create failed!\n");
        avcap_close(avcap);
        return -1;
    }
    printf("avcap info: %s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, avcap->conf.video.width, avcap->conf.video.height,
        avcap->conf.video.fps.num, avcap->conf.video.fps.den, pixel_format_to_string(avcap->conf.video.format));
    fp = file_open(OUTPUT_DSHOW, F_CREATE);
    avcap_start_stream(avcap, on_frame);

    sleep(5);
    avcap_stop_stream(avcap);
    file_close(fp);
    video_frame_destroy(frm);
    avcap_close(avcap);
    printf("write %s fininshed!\n", OUTPUT_DSHOW);
    printf("======== dshow_test leave\n");
    return 0;
}

static int pulseaudio_test()
{
#if defined (OS_LINUX)
    struct avcap_ctx *avcap;
    struct avcap_config conf = {
            .type = AVCAP_TYPE_AUDIO,
            .backend = AVCAP_BACKEND_PULSEAUDIO,
            .audio = {
                .format = SAMPLE_FORMAT_PCM_S16LE,
                .sample_rate = 44100,
                .channels = 2,
            }
    };
    printf("======== pulseaudio_test enter\n");
    avcap = avcap_open("xxx", &conf);
    if (!avcap) {
        printf("avcap_open pa failed!\n");
        return -1;
    }
    printf("sample_rate: %d\n", avcap->conf.audio.sample_rate);
    printf("    channel: %d\n", avcap->conf.audio.channels);
    printf("     format: %s\n", sample_format_to_string(avcap->conf.audio.format));
    printf("     device: %s\n", avcap->conf.audio.device);
    fp = file_open(OUTPUT_PA, F_CREATE);
    avcap_start_stream(avcap, on_frame);
    sleep(5);
    avcap_stop_stream(avcap);
    file_close(fp);
    avcap_close(avcap);
    printf("write %s fininshed!\n", OUTPUT_PA);
    printf("======== pulseaudio_test leave\n");
#endif
    return 0;
}


static int xcb_test()
{
#if defined (OS_LINUX)
    struct avcap_config conf = {
            .type = AVCAP_TYPE_VIDEO,
            .backend = AVCAP_BACKEND_XCB,
    };
    struct avcap_ctx *avcap = avcap_open(NULL, &conf);
    if (!avcap) {
        printf("avcap_open uvc failed!\n");
        return -1;
    }
#endif
    return 0;
}

int main(int argc, char **argv)
{
    v4l2_test();
    dummy_test();
    uvc_test();
    dshow_test();
    pulseaudio_test();
    xcb_test();
    return 0;
}
