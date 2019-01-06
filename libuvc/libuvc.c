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
#include <libmacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

/*
 * refer to /usr/include/linux/v4l2-controls.h
 * supported v4l2 control cmds
 */
static const uint32_t v4l2_cid_supported[] = {
    V4L2_CID_BRIGHTNESS,
    V4L2_CID_CONTRAST,
    V4L2_CID_SATURATION,
    V4L2_CID_HUE,
    V4L2_CID_AUTO_WHITE_BALANCE,
    V4L2_CID_GAMMA,
    V4L2_CID_POWER_LINE_FREQUENCY,
    V4L2_CID_WHITE_BALANCE_TEMPERATURE,
    V4L2_CID_SHARPNESS,
    V4L2_CID_BACKLIGHT_COMPENSATION,
    V4L2_CID_LASTP1
};

#define MAX_V4L2_CID        (sizeof(v4l2_cid_supported)/sizeof(uint32_t))
#define MAX_V4L_BUF         (32)
#define MAX_V4L_REQBUF_CNT  (256)

struct frame {
    struct iovec buf;
    int index;
};

struct v4l2_ctx {
    int fd;
    char *name;
    int width;
    int height;
    struct iovec buf[MAX_V4L_BUF];
    int buf_index;
    int req_count;
    bool qbuf_done;
    struct v4l2_capability cap;
    uint32_t ctrl_flags;
    struct v4l2_queryctrl controls[MAX_V4L2_CID];
    struct uvc_ctx *parent;
};

static int v4l2_get_cap(struct v4l2_ctx *vc)
{
    if (-1 == ioctl(vc->fd, VIDIOC_QUERYCAP, &vc->cap)) {
        printf("failed to VIDIOC_QUERYCAP\n");
        return -1;
    }

    printf("[V4L2 Capability Information]:\n");
    printf("\tcap.driver: \t\"%s\"\n", vc->cap.driver);
    printf("\tcap.card: \t\"%s\"\n", vc->cap.card);
    printf("\tcap.bus_info: \t\"%s\"\n", vc->cap.bus_info);
    printf("\tcap.version: \t\"%d\"\n", vc->cap.version);
    printf("\tcap.capabilities:\n");
    if (vc->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        printf("\t\t\t VIDEO_CAPTURE\n");
    if (vc->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        printf("\t\t\t VIDEO_OUTPUT\n");
    if (vc->cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
        printf("\t\t\t VIDEO_OVERLAY\n");
    if (vc->cap.capabilities & V4L2_CAP_VBI_CAPTURE)
        printf("\t\t\t VBI_CAPTURE\n");
    if (vc->cap.capabilities & V4L2_CAP_VBI_OUTPUT)
        printf("\t\t\t VBI_OUTPUT\n");
    if (vc->cap.capabilities & V4L2_CAP_RDS_CAPTURE)
        printf("\t\t\t RDS_CAPTURE\n");
    if (vc->cap.capabilities & V4L2_CAP_TUNER)
        printf("\t\t\t TUNER\n");
    if (vc->cap.capabilities & V4L2_CAP_AUDIO)
        printf("\t\t\t AUDIO\n");
    if (vc->cap.capabilities & V4L2_CAP_READWRITE)
        printf("\t\t\t READWRITE\n");
    if (vc->cap.capabilities & V4L2_CAP_ASYNCIO)
        printf("\t\t\t ASYNCIO\n");
    if (vc->cap.capabilities & V4L2_CAP_STREAMING)
        printf("\t\t\t STREAMING\n");
    if (vc->cap.capabilities & V4L2_CAP_TIMEPERFRAME)
        printf("\t\t\t TIMEPERFRAME\n");
    if (!(vc->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        printf("Device does not support capturing.\n");
        return -1;
    }
    return 0;
}

static char *v4l2_fourcc_parse(char *buf, int len, uint32_t pix)
{
    if (len < 5) {
        return NULL;
    }
    snprintf(buf, len, "%c%c%c%c", (pix&0x000000FF),
                                   (pix&0x0000FF00)>>8,
                                   (pix&0x00FF0000)>>16,
                                   (pix&0xFF000000)>>24);
    return buf;
}

static void v4l2_get_fmt(struct v4l2_ctx *vc)
{
    struct v4l2_fmtdesc fmtdesc;
    char pixel[5] = {0};
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("[V4L2 Support Format]:\n");
    while (-1 != ioctl(vc->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        printf("\t%d. [%s] \"%s\"\n", fmtdesc.index,
               v4l2_fourcc_parse(pixel, sizeof(pixel), fmtdesc.pixelformat),
               fmtdesc.description);
        ++fmtdesc.index;
    }
}

static int v4l2_get_input(struct v4l2_ctx *vc)
{
    struct v4l2_input input;
    struct v4l2_standard standard;
    v4l2_std_id std_id;
    memset(&input, 0, sizeof(input));
    input.index = 0;
    if (-1 == ioctl(vc->fd, VIDIOC_ENUMINPUT, &input)) {
        printf("Unable to query input %d VIDIOC_ENUMINPUT,"
               "if you use a WEBCAM change input value in conf by -1",
              input.index);
        return -1;
    }
    printf("[V4L2 Input information]:\n");
    printf("\tinput.name: \t\"%s\"\n", input.name);
    if (input.type & V4L2_INPUT_TYPE_TUNER)
        printf("\t\t\t TUNER\n");
    if (input.type & V4L2_INPUT_TYPE_CAMERA)
        printf("\t\t\t CAMERA\n");
    if (-1 != ioctl(vc->fd, VIDIOC_G_STD, &std_id)) {
        memset(&standard, 0, sizeof(standard));
        standard.index = 0;
        while (ioctl(vc->fd, VIDIOC_ENUMSTD, &standard) == 0) {
            if (standard.id & std_id)
                printf("\t\t- video standard %s\n", standard.name);
            standard.index++;
        }
        printf("Set standard method %d\n", (int)std_id);
    }
    return 0;
}

static void v4l2_get_cid(struct v4l2_ctx *vc)
{
    int i;
    struct v4l2_queryctrl *qctrl;
    struct v4l2_control control;
    printf("[V4L2 Supported Control]:\n");
    for (i = 0; v4l2_cid_supported[i] != V4L2_CID_LASTP1; i++) {
        qctrl = &vc->controls[i];
        qctrl->id = v4l2_cid_supported[i];
        if (-1 == ioctl(vc->fd, VIDIOC_QUERYCTRL, qctrl)) {
            continue;
        }
        vc->ctrl_flags |= 1 << i;
        memset(&control, 0, sizeof (control));
        control.id = v4l2_cid_supported[i];
        if (-1 == ioctl(vc->fd, VIDIOC_G_CTRL, &control)) {
            continue;
        }
        printf("\t%s, range: [%d, %d], default: %d, current: %d\n",
             qctrl->name,
             qctrl->minimum, qctrl->maximum,
             qctrl->default_value, control.value);
    }
}

int uvc_print_info(struct uvc_ctx *c)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)c->opaque;
    v4l2_get_input(vc);
    v4l2_get_cap(vc);
    v4l2_get_fmt(vc);
    v4l2_get_cid(vc);
    return 0;
}

static int v4l2_set_format(struct v4l2_ctx *vc)
{
    struct v4l2_format fmt;
    struct v4l2_pix_format *pix = &fmt.fmt.pix;
    char pixel[5] = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_G_FMT, &fmt)) {
        printf("%s ioctl(VIDIOC_G_FMT) failed: %d\n", __func__, errno);
        return -1;
    }
    if (pix->pixelformat != V4L2_PIX_FMT_YUYV) {
        pix->pixelformat = V4L2_PIX_FMT_YUYV;
        printf("change pixel format from %s to %s\n",
                v4l2_fourcc_parse(pixel, sizeof(pixel), pix->pixelformat),
                v4l2_fourcc_parse(pixel, sizeof(pixel), V4L2_PIX_FMT_YUYV));
    }
    if (vc->width > 0 || vc->height > 0) {
        pix->width = vc->width;
        pix->height = vc->height;
    } else {
        vc->width = pix->width;
        vc->height = pix->height;
    }
    if (-1 == ioctl(vc->fd, VIDIOC_S_FMT, &fmt)) {
        printf("%s ioctl(VIDIOC_S_FMT) failed: %d\n", __func__, errno);
        return -1;
    }
    return 0;
}

static int v4l2_req_buf(struct v4l2_ctx *vc)
{
    int i;
    enum v4l2_buf_type type;
    struct v4l2_requestbuffers req = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .count = MAX_V4L_REQBUF_CNT,
        .memory = V4L2_MEMORY_MMAP
    };
    //request buffer
    if (-1 == ioctl(vc->fd, VIDIOC_REQBUFS, &req)) {
        printf("%s ioctl(VIDIOC_REQBUFS) failed: %d\n", __func__, errno);
        return -1;
    }
    vc->req_count = req.count;
    if (req.count > MAX_V4L_REQBUF_CNT || req.count < 2) {
        printf("Insufficient buffer memory\n");
        return -1;
    }
    for (i = 0; i < vc->req_count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        //query buffer
        if (-1 == ioctl(vc->fd, VIDIOC_QUERYBUF, &buf)) {
            printf("%s ioctl(VIDIOC_QUERYBUF) failed: %d\n", __func__, errno);
            return -1;
        }
        //mmap buffer
        vc->buf[i].iov_len = buf.length;
        vc->buf[i].iov_base = mmap(NULL, buf.length, PROT_READ|PROT_WRITE,
                               MAP_SHARED, vc->fd, buf.m.offset);
        if (MAP_FAILED == vc->buf[i].iov_base) {
            printf("mmap failed: %d\n", errno);
            return -1;
        }
        //enqueue buffer
        if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
            printf("%s ioctl(VIDIOC_QBUF) failed: %d\n", __func__, errno);
            return -1;
        }
    }
    vc->qbuf_done = true;
    //stream on
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vc->fd, VIDIOC_STREAMON, &type)) {
        printf("%s ioctl(VIDIOC_STREAMON) failed: %d\n", __func__, errno);
        return -1;
    }
    return 0;
}

static struct v4l2_ctx *v4l2_open(const char *dev, int width, int height)
{
    int fd = -1;
    struct v4l2_ctx *vc = CALLOC(1, struct v4l2_ctx);
    if (!vc) {
        printf("malloc v4l2_ctx failed!\n");
        return NULL;
    }
    fd = open(dev, O_RDWR);
    if (fd == -1) {
        printf("open %s failed: %d\n", dev, errno);
        goto failed;
    }
    vc->name = strdup(dev);
    vc->fd = fd;
    vc->width = width;
    vc->height = height;

    if (-1 == v4l2_set_format(vc)) {
        printf("v4l2_set_format failed\n");
        goto failed;
    }
    if (-1 == v4l2_req_buf(vc)) {
        printf("v4l2_req_buf failed\n");
        goto failed;
    }
    return vc;

failed:
    if (fd != -1) {
        close(fd);
    }
    if (vc->name) {
        free(vc->name);
    }
    if (vc) {
        free(vc);
    }
    return NULL;
}

static int v4l2_buf_enqueue(struct v4l2_ctx *vc)
{
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = vc->buf_index
    };

    if (vc->qbuf_done) {
        return 0;
    }
    if (-1 == ioctl(vc->fd, VIDIOC_QBUF, &buf)) {
        printf("%s ioctl(VIDIOC_QBUF) failed: %d\n", __func__, errno);
        return -1;
    }
    vc->qbuf_done = true;
    return 0;
}

static int v4l2_buf_dequeue(struct v4l2_ctx *vc, struct frame *f)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (!vc->qbuf_done) {
        printf("v4l2 need VIDIOC_QBUF first!\n");
        return -1;
    }
    while (1) {
        if (-1 == ioctl(vc->fd, VIDIOC_DQBUF, &buf)) {
            printf("%s ioctl(VIDIOC_DQBUF) failed: %d\n", __func__, errno);
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            else
                return -1;
        }
        break;
    }
    vc->qbuf_done = false;
    vc->buf_index = buf.index;
    memcpy(&vc->parent->timestamp, &buf.timestamp, sizeof(struct timeval));
    f->index = buf.index;
    f->buf.iov_base = vc->buf[buf.index].iov_base;
    f->buf.iov_len = buf.bytesused;
    return f->buf.iov_len;
}

static int v4l2_read(struct v4l2_ctx *vc, void *buf, int len)
{
    struct frame f;
    int flen;

    flen = v4l2_buf_dequeue(vc, &f);
    if (flen == -1) {
        printf("v4l2 dequeue failed!\n");
        return -1;
    }
    if (flen > len) {
        printf("v4l2 frame is %d bytes, but buffer len is %d, not enough!\n",
             flen, len);
        return -1;
    }
    if (len < (int)f.buf.iov_len) {
        printf("error occur!\n");
        return -1;
    }
    memcpy(buf, f.buf.iov_base, f.buf.iov_len);
    return f.buf.iov_len;
}

static int v4l2_write(struct v4l2_ctx *vc)
{
    return v4l2_buf_enqueue(vc);
}

static void v4l2_close(struct v4l2_ctx *vc)
{
    int i;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == ioctl(vc->fd, VIDIOC_STREAMOFF, &type)) {
        printf("%s ioctl(VIDIOC_STREAMOFF) failed: %d\n", __func__, errno);
    }
    for (i = 0; i < vc->req_count; i++) {
        munmap(vc->buf[i].iov_base, vc->buf[i].iov_len);
    }
    free(vc->name);
    close(vc->fd);
    free(vc);
}

struct uvc_ctx *uvc_open(const char *dev, int width, int height)
{
    struct uvc_ctx *uvc = CALLOC(1, struct uvc_ctx);
    if (!uvc) {
        printf("malloc failed!\n");
        return NULL;
    }
    struct v4l2_ctx *vc = v4l2_open(dev, width, height);
    if (!vc) {
        printf("v4l2_open %s failed!\n", dev);
        goto failed;
    }
    vc->parent = uvc;
    uvc->width = width;
    uvc->height = height;
    uvc->opaque = vc;
    return uvc;
failed:
    free(uvc);
    return NULL;
}

int uvc_read(struct uvc_ctx *uvc, void *buf, size_t len)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;
    if (-1 == v4l2_write(vc)) {
        return -1;
    }
    return v4l2_read(vc, buf, len);
}

static int v4l2_set_control(struct v4l2_ctx *vc, uint32_t cid, int value)
{
    int i;
    struct v4l2_control control;
    int ret = -1;
    for (i = 0; v4l2_cid_supported[i]; i++) {
        if (!(vc->ctrl_flags & (1 << i))) {
            continue;
        }
        if (cid != v4l2_cid_supported[i]) {
            continue;
        }
        struct v4l2_queryctrl *ctrl = &vc->controls[i];

        memset(&control, 0, sizeof (control));
        control.id = v4l2_cid_supported[i];
        switch (ctrl->type) {
        case V4L2_CTRL_TYPE_INTEGER://0~255
            value = control.value =
                    (value * (ctrl->maximum - ctrl->minimum) / 256) + ctrl->minimum;
            ret = ioctl(vc->fd, VIDIOC_S_CTRL, &control);
            break;

        case V4L2_CTRL_TYPE_BOOLEAN:
            value = control.value = value ? 1 : 0;
            ret = ioctl(vc->fd, VIDIOC_S_CTRL, &control);
            break;

        default:
            printf("control type not supported yet");
            return -1;
        }
        printf("setting %s to %d\n", ctrl->name, value);
    }
    return ret;
}

int uvc_ioctl(struct uvc_ctx *c, uint32_t cmd, void *buf, int len)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)c->opaque;
    struct video_ctrl *vctrl;
    switch (cmd) {
    case UVC_GET_CAP:
        v4l2_get_cap(vc);
        break;
    case UVC_SET_CTRL:
        vctrl = (struct video_ctrl *)buf;
        v4l2_set_control(vc, vctrl->cmd, vctrl->val);
        break;
    default:
        printf("cmd %d not supported yet!\n", cmd);
        break;
    }
    return 0;
}

void uvc_close(struct uvc_ctx *uvc)
{
    if (!uvc) {
        return;
    }
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;
    v4l2_close(vc);
    free(uvc);
}
