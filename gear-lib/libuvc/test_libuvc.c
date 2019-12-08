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

#if defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#pragma comment(lib , "libposix4win.lib")
#pragma comment(lib , "libfile.lib")
#endif

int main(int argc, char **argv)
{
    int i = 0;
    int flen = 2 * 640 * 480;
    int size = 0;
    struct file *fp;
    void *frm = calloc(1, flen);
    struct uvc_ctx *uvc = uvc_open("/dev/video0", 640, 480);
    uvc_print_info(uvc);
    fp = file_open("uvc.yuv", F_CREATE);
    uvc_start_stream(uvc);
    for (i = 0; i < 64; ++i) {
        memset(frm, 0, flen);
        size = uvc_read(uvc, frm, flen);
        if (size == -1) {
            continue;
        }
        file_write(fp, frm, size);
    }
    file_close(fp);
    uvc_ioctl(uvc, UVC_GET_CAP, NULL, 0);
    uvc_stop_stream(uvc);
    uvc_close(uvc);
    return 0;
}
