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
#include "libuvc.h"
#include <libfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define VIDEO_DEV       "/dev/video0"
#define VIDEO_WIDTH     640
#define VIDEO_HEIGHT    480
#define OUTPUT_V4L2     "v4l2.yuv"
#define OUTPUT_DUMMY    "dummy.yuv"

static struct file *fp;

static int on_frame(struct uvc_ctx *c, struct video_frame *frm)
{
    static uint64_t last_ms = 0;

    printf("frm[%" PRIu64 "] size=%" PRIu64 ", ts=%" PRIu64 " ms, gap=%" PRIu64 " ms\n",
          frm->frame_id, frm->total_size, frm->timestamp/1000000, frm->timestamp/1000000 - last_ms);
    last_ms = frm->timestamp/1000000;
    file_write(fp, frm->data[0], frm->total_size);
    return 0;
}

int v4l2_test()
{
    struct video_frame *frm;
    struct uvc_config conf = {
        .width  = VIDEO_WIDTH,
        .height = VIDEO_HEIGHT,
        .fps    = {30, 1},
        .format = PIXEL_FORMAT_YUY2,
    };
    struct uvc_ctx *uvc = uvc_open(UVC_TYPE_V4L2, VIDEO_DEV, &conf);
    if (!uvc) {
        printf("uvc_open failed!\n");
        return -1;
    }
    //uvc_ioctl(uvc, UVC_SET_CONF, &conf);
    uvc_ioctl(uvc, UVC_GET_CAP, NULL);
    frm = video_frame_create(uvc->conf.format, uvc->conf.width, uvc->conf.height, MEDIA_MEM_SHALLOW);
    if (!frm) {
        printf("video_frame_create failed!\n");
        uvc_close(uvc);
        return -1;
    }
    printf("uvc info: %s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, uvc->conf.width, uvc->conf.height,
        uvc->conf.fps.num, uvc->conf.fps.den, pixel_format_to_string(uvc->conf.format));
    fp = file_open(OUTPUT_V4L2, F_CREATE);
    uvc_start_stream(uvc, on_frame);
    sleep(5);
    uvc_stop_stream(uvc);
    file_close(fp);
    video_frame_destroy(frm);
    uvc_close(uvc);
    printf("write %s fininshed!\n", OUTPUT_V4L2);
    return 0;
}

int dummy_test()
{
    struct video_frame *frm;
    struct uvc_config conf = {
        .width  = 320,
        .height = 240,
        .fps    = {30, 1},
        .format = PIXEL_FORMAT_YUY2,
    };
    struct uvc_ctx *uvc = uvc_open(UVC_TYPE_DUMMY, "sample_yuv422p.yuv", &conf);
    if (!uvc) {
        printf("uvc_open failed!\n");
        return -1;
    }
    frm = video_frame_create(uvc->conf.format, uvc->conf.width, uvc->conf.height, MEDIA_MEM_SHALLOW);
    if (!frm) {
        printf("video_frame_create failed!\n");
        uvc_close(uvc);
        return -1;
    }
    printf("%s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, uvc->conf.width, uvc->conf.height,
        uvc->conf.fps.num, uvc->conf.fps.den, pixel_format_to_string(uvc->conf.format));
    fp = file_open(OUTPUT_DUMMY, F_CREATE);
    uvc_start_stream(uvc, on_frame);
    sleep(5);
    uvc_stop_stream(uvc);
    file_sync(fp);
    file_close(fp);
    video_frame_destroy(frm);
    uvc_close(uvc);
    printf("write %s fininshed!\n", OUTPUT_DUMMY);
    return 0;
}

int main(int argc, char **argv)
{
    v4l2_test();
    //dummy_test();
    return 0;
}
