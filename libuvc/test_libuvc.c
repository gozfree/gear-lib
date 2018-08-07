/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#include "libuvc.h"
#include <libfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int flen = 2 * 640 * 480;
    int size = 0;
    void *frm = calloc(1, flen);
    struct uvc_ctx *uvc = uvc_open("/dev/video0", 640, 480);
    struct file *fp = file_open("uvc.yuv", F_CREATE);
    for (int i = 0; i < 20; ++i) {
        memset(frm, 0, flen);
        size = uvc_read(uvc, frm, flen);
        file_write(fp, frm, size);
    }
    file_close(fp);
    uvc_close(uvc);
    return 0;
}
