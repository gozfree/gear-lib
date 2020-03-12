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
#include <gear-lib/libfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#define VIDEO_DEV       "/dev/video0"
#define VIDEO_WIDTH     640
#define VIDEO_HEIGHT    480

int main(int argc, char **argv)
{
    int i = 0;
    int size = 0;
    struct file *fp;
    struct video_frame *frm;
    struct uvc_ctx *uvc = uvc_open(VIDEO_DEV, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (!uvc) {
        printf("uvc_open failed!\n");
        return -1;
    }
    frm = video_frame_create(uvc->format, uvc->width, uvc->height, VFC_NONE);
    if (!frm) {
        printf("video_frame_create failed!\n");
        uvc_close(uvc);
        return -1;
    }
    printf("%s %dx%d@%d/%d fps format:%s\n", VIDEO_DEV, uvc->width, uvc->height,
        uvc->fps_num, uvc->fps_den, video_format_name(uvc->format));
    //uvc_ioctl(uvc, UVC_GET_CAP, NULL, 0);
    fp = file_open("uvc.yuv", F_CREATE);
    uvc_start_stream(uvc, NULL);
    for (i = 0; i < 32; ++i) {
        size = uvc_query_frame(uvc, frm);
        if (size == -1) {
            continue;
        }
        file_write(fp, frm->data[0], frm->total_size);
        printf("frm[%zu] size=%zu, ts=%zu ms\n", frm->id, frm->total_size, frm->timestamp/1000000);
    }
    video_frame_destroy(frm);
    file_close(fp);
    uvc_stop_stream(uvc);
    uvc_close(uvc);
    return 0;
}
