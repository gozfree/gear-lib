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
#define OUTPUT_V4L2     "v4l2.yuv"
#define OUTPUT_DUMMY    "dummy.yuv"

static struct file *fp;


static int on_frame(struct avcap_ctx *c, struct media_frame *frm)
{
    static uint64_t last_ms = 0;
    static int luma = 0;
    static int i = 0;

    printf("frm[%" PRIu64 "] size=%" PRIu64 ", ts=%" PRIu64 " ms, gap=%" PRIu64 " ms\n",
          frm->video.frame_id, frm->video.total_size, frm->video.timestamp/1000000, frm->video.timestamp/1000000 - last_ms);
    last_ms = frm->video.timestamp/1000000;
    luma = 2 * i++;
    luma *= i%2 ? 1: -1;
    avcap_ioctl(c, VIDCAP_SET_LUMA, luma);
    file_write(fp, frm->video.data[0], frm->video.total_size);
    return 0;
}

int v4l2_test()
{
	struct avcap_ctx *avcap;
    struct video_frame *frm;
    struct avcap_config conf = {
            .type = AVCAP_TYPE_VIDEO,
            .video = {
                PIXEL_FORMAT_YUY2,
                VIDEO_WIDTH,
                VIDEO_HEIGHT,
                .fps = {30, 1},
            },
    };
    avcap = avcap_open(VIDEO_DEV, &conf);
    if (!avcap) {
        printf("avcap_open failed!\n");
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
    return 0;
}

int dummy_test()
{
#if defined (OS_LINUX)
    struct video_frame *frm;
    struct avcap_config conf = {
        .type = AVCAP_TYPE_VIDEO,
        .video = {
            PIXEL_FORMAT_YUY2,
            320,
            240,
            .fps = {30, 1},
            "sample_320x240_yuv422p.yuv",
        }
    };
    struct avcap_ctx *avcap = avcap_open(NULL, &conf);
    if (!avcap) {
        printf("avcap_open failed!\n");
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
#endif
    return 0;
}

int uvc_test()
{
#if defined (OS_LINUX)
    struct video_frame *frm;
    struct avcap_config conf = {
            .type = AVCAP_TYPE_VIDEO,
            .video = {
                PIXEL_FORMAT_YUY2,
                VIDEO_WIDTH,
                VIDEO_HEIGHT,
                .fps = {30, 1},
            },
    };
    struct avcap_ctx *avcap = avcap_open(VIDEO_DEV, &conf);
    if (!avcap) {
        printf("avcap_open failed!\n");
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
#endif
    return 0;
}
int main(int argc, char **argv)
{
    //v4l2_test();
    //dummy_test();
    uvc_test();
    return 0;
}
