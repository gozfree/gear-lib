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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

/*
 * HAVE_LIBV4L2 will build with libv4l2 (v4l-utils) functions
 */
#if HAVE_LIBV4L2
#include <libv4l2.h>
#endif

int (*open_f)(const char *file, int oflag, ...);
int (*close_f)(int fd);
int (*dup_f)(int fd);
int (*ioctl_f)(int fd, unsigned long int request, ...);
ssize_t (*read_f)(int fd, void *buffer, size_t n);
void *(*mmap_f)(void *start, size_t length, int prot, int flags, int fd, int64_t offset);
int (*munmap_f)(void *_start, size_t length);

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

#define MAX_V4L2_CID             (sizeof(v4l2_cid_supported)/sizeof(uint32_t))
#define MAX_V4L_BUF              (32)
#define MAX_V4L_REQBUF_CNT       (4)
#define MAX_V4L2_DQBUF_RETYR_CNT (5)

struct v4l2_ctx {
    int fd;
    int channel; /*one video node may contain several input channel */
    int standard;
    uint32_t pixfmt;
    uint32_t linesize;
    int dv_timing;
    char *name;
    uint32_t width;
    uint32_t height;
    uint32_t fps_num;
    uint32_t fps_den;
    struct iovec buf[MAX_V4L_BUF];
    int buf_index;
    int req_count;
    bool qbuf_done;
    uint64_t first_ts;
    uint64_t prev_ts;
    uint64_t frame_id;
    struct v4l2_capability cap;
    uint32_t ctrl_flags;
    struct v4l2_queryctrl controls[MAX_V4L2_CID];
    struct uvc_ctx *parent;
};

static int uvc_v4l2_init(struct v4l2_ctx *c);
static int uvc_v4l2_create_mmap(struct v4l2_ctx *c);
static int uvc_v4l2_set_format(int fd, uint32_t *w, uint32_t *h, uint32_t *pixelformat, uint32_t *bytesperline);
static int uvc_v4l2_set_framerate(int fd, uint32_t *fps_num, uint32_t *fps_den);
static int uvc_v4l2_start_stream(struct uvc_ctx *uvc);


#define V4L2_FOURCC_STR(code)                                               \
        (char[5])                                                           \
{                                                                           \
    (code&0xFF), ((code>>8)&0xFF), ((code>>16)&0xFF), ((code>>24)&0xFF), 0  \
}

#define timeval2ns(tv) \
    (((uint64_t)tv.tv_sec * 1000000000) + ((uint64_t)tv.tv_usec * 1000))


static void *uvc_v4l2_open(struct uvc_ctx *uvc, const char *dev, uint32_t width, uint32_t height)
{
    int fd = -1;
    struct v4l2_ctx *c = calloc(1, sizeof(struct v4l2_ctx));
    if (!c) {
        printf("malloc v4l2_ctx failed!\n");
        return NULL;
    }

#define SET_WRAPPERS(prefix) do {    \
    open_f   = prefix ## open;       \
    close_f  = prefix ## close;      \
    dup_f    = prefix ## dup;        \
    ioctl_f  = prefix ## ioctl;      \
    read_f   = prefix ## read;       \
    mmap_f   = prefix ## mmap;       \
    munmap_f = prefix ## munmap;     \
} while (0)

#if HAVE_LIBV4L2
        SET_WRAPPERS(v4l2_);
#else
        SET_WRAPPERS();
#endif

#define v4l2_open   open_f
#define v4l2_close  close_f
#define v4l2_dup    dup_f
#define v4l2_ioctl  ioctl_f
#define v4l2_read   read_f
#define v4l2_mmap   mmap_f
#define v4l2_munmap munmap_f

    fd = v4l2_open(dev, O_RDWR);
    if (fd == -1) {
        printf("open %s failed: %d\n", dev, errno);
        goto failed;
    }
    c->name = strdup(dev);
    c->fd = fd;

    c->channel = -1;
    c->standard = -1;
    c->dv_timing = -1;
    c->width = width;
    c->height = height;
    c->fps_num = -1;//unlimit fps
    c->fps_den = 1;
    c->frame_id = 0;

    if (-1 == uvc_v4l2_init(c)) {
        printf("uvc_v4l2_init failed\n");
        goto failed;
    }

    if (uvc_v4l2_set_format(c->fd, &width, &height, &c->pixfmt, &c->linesize) < 0) {
        printf("uvc_v4l2_set_format failed\n");
        goto failed;
    }

    if (uvc_v4l2_set_framerate(c->fd, &c->fps_num, &c->fps_den) < 0) {
        printf("uvc_v4l2_set_framerate failed\n");
        goto failed;
    }

    if (uvc_v4l2_create_mmap(c) < 0) {
        printf("uvc_v4l2_create_mmap failed\n");
        goto failed;
    }

    uvc->width = c->width;
    uvc->height = c->height;
    uvc->fps_num = c->fps_num;
    uvc->fps_den = c->fps_den;
    uvc->format = video_format_from_fourcc(c->pixfmt);
    uvc->fd = fd;
    c->parent = uvc;
    return c;

failed:
    if (fd != -1) {
        v4l2_close(fd);
    }
    if (c->name) {
        free(c->name);
    }
    if (c) {
        free(c);
    }
    return NULL;
}

static int v4l2_set_standard(int fd, int *standard)
{
    if (*standard == -1) {
        if (v4l2_ioctl(fd, VIDIOC_G_STD, standard) < 0)
            return -1;
    } else {
        if (v4l2_ioctl(fd, VIDIOC_S_STD, standard) < 0)
            return -1;
    }

    return 0;
}

static int v4l2_enum_dv_timing(int dev, struct v4l2_dv_timings *dvt,
                int index)
{
#if !defined(VIDIOC_ENUM_DV_TIMINGS) || !defined(V4L2_IN_CAP_DV_TIMINGS)
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(dvt);
    UNUSED_PARAMETER(index);
    return -1;
#else
    if (!dev || !dvt)
        return -1;

    struct v4l2_enum_dv_timings iter;
    memset(&iter, 0, sizeof(iter));
    iter.index = index;

    if (v4l2_ioctl(dev, VIDIOC_ENUM_DV_TIMINGS, &iter) < 0)
        return -1;

    memcpy(dvt, &iter.timings, sizeof(struct v4l2_dv_timings));

    return 0;
#endif
}

static int v4l2_set_dv_timing(int fd, int *timing)
{
    if (*timing == -1)
        return 0;

    struct v4l2_dv_timings dvt;

    if (v4l2_enum_dv_timing(fd, &dvt, *timing) < 0)
        return -1;

    if (v4l2_ioctl(fd, VIDIOC_S_DV_TIMINGS, &dvt) < 0)
        return -1;

    return 0;
}

static int uvc_v4l2_init(struct v4l2_ctx *c)
{
    struct v4l2_input in;
    memset(&in, 0, sizeof(in));

    if (v4l2_ioctl(c->fd, VIDIOC_G_INPUT, &in.index) < 0) {
        printf("ioctl VIDIOC_G_INPUT failed\n");
        return -1;
    }

    if (v4l2_ioctl(c->fd, VIDIOC_ENUMINPUT, &in) < 0) {
        printf("ioctl VIDIOC_ENUMINPUT failed\n");
        return -1;
    }

    if (in.capabilities & V4L2_IN_CAP_STD) {
        if (v4l2_set_standard(c->fd, &c->standard) < 0) {
            printf("Unable to set video standard\n");
            return -1;
        }
    }

    if (in.capabilities & V4L2_IN_CAP_DV_TIMINGS) {
        if (v4l2_set_dv_timing(c->fd, &c->dv_timing) < 0) {
            printf("Unable to set dv timing\n");
            return -1;
        }
    }
    return 0;
}

static int uvc_v4l2_set_format(int fd, uint32_t *width, uint32_t *height,
                uint32_t *pixelformat, uint32_t *bytesperline)
{
    struct v4l2_format fmt;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        printf("%s ioctl(VIDIOC_G_FMT) failed: %d\n", __func__, errno);
        return -1;
    }

    fmt.fmt.pix.width = *width;
    fmt.fmt.pix.height = *height;
    fmt.fmt.pix.pixelformat = *pixelformat;

    if (v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
        return -1;

    if (fmt.fmt.pix.width != *width ||
        fmt.fmt.pix.height != *height) {
        printf("v4l2 format force from %d*%d to %d*%d\n",
                *width, *height, fmt.fmt.pix.width,fmt.fmt.pix.height);
    }

    *width = fmt.fmt.pix.width;
    *height = fmt.fmt.pix.height;
    *pixelformat = fmt.fmt.pix.pixelformat;
    *bytesperline = fmt.fmt.pix.bytesperline;
    return 0;
}

static int uvc_v4l2_set_framerate(int fd, uint32_t *fps_num, uint32_t *fps_den)
{
    struct v4l2_streamparm par;

    par.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_G_PARM, &par) < 0)
        return -1;

    par.parm.capture.timeperframe.numerator = *fps_den;
    par.parm.capture.timeperframe.denominator = *fps_num;

    if (v4l2_ioctl(fd, VIDIOC_S_PARM, &par) < 0)
        return -1;

    if (par.parm.capture.timeperframe.numerator != *fps_den ||
        par.parm.capture.timeperframe.denominator != *fps_num) {
        /*
        printf("v4l2 framerate force from %d/%d to %d/%d\n",
                *fps_num, *fps_den,
                par.parm.capture.timeperframe.denominator,
                par.parm.capture.timeperframe.numerator);
        */
    }
    *fps_den = par.parm.capture.timeperframe.numerator;
    *fps_num = par.parm.capture.timeperframe.denominator;
    return 0;
}

static int uvc_v4l2_start_stream(struct uvc_ctx *uvc)
{
    enum v4l2_buf_type type;
    struct v4l2_buffer enq;
    struct v4l2_ctx *c = (struct v4l2_ctx *)uvc->opaque;

    memset(&enq, 0, sizeof(enq));
    enq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enq.memory = V4L2_MEMORY_MMAP;

    for (enq.index = 0; enq.index < c->req_count; ++enq.index) {
        if (v4l2_ioctl(c->fd, VIDIOC_QBUF, &enq) < 0) {
            printf("unable to queue buffer\n");
            return -1;
        }
    }
    c->qbuf_done = true;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(c->fd, VIDIOC_STREAMON, &type) < 0) {
        printf("unable to start stream\n");
        return -1;
    }

    return 0;
}

static int uvc_v4l2_stop_stream(struct uvc_ctx *uvc)
{
    int i;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_ctx *c = (struct v4l2_ctx *)uvc->opaque;

    if (v4l2_ioctl(c->fd, VIDIOC_STREAMOFF, &type) < 0) {
        printf("%s ioctl(VIDIOC_STREAMOFF) failed: %d\n", __func__, errno);
    }
    for (i = 0; i < c->req_count; i++) {
        v4l2_munmap(c->buf[i].iov_base, c->buf[i].iov_len);
    }
    return 0;
}

static int uvc_v4l2_create_mmap(struct v4l2_ctx *c)
{
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers req = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .count = MAX_V4L_REQBUF_CNT,
        .memory = V4L2_MEMORY_MMAP
    };
    //request buffer
    if (v4l2_ioctl(c->fd, VIDIOC_REQBUFS, &req) < 0) {
        printf("%s ioctl(VIDIOC_REQBUFS) failed: %d\n", __func__, errno);
        return -1;
    }
    c->req_count = req.count;
    if (req.count > MAX_V4L_REQBUF_CNT || req.count < 2) {
        printf("Insufficient buffer memory\n");
        return -1;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = req.type;
    buf.memory = req.memory;

    for (buf.index = 0; buf.index < c->req_count; ++buf.index) {
        //query buffer
        if (v4l2_ioctl(c->fd, VIDIOC_QUERYBUF, &buf) < 0) {
            printf("%s ioctl(VIDIOC_QUERYBUF) failed: %d\n", __func__, errno);
            return -1;
        }
        //mmap buffer
        c->buf[buf.index].iov_len = buf.length;
        c->buf[buf.index].iov_base =
                v4l2_mmap(NULL, buf.length, PROT_READ|PROT_WRITE,
                                MAP_SHARED, c->fd, buf.m.offset);
        if (MAP_FAILED == c->buf[buf.index].iov_base) {
            printf("mmap failed: %d\n", errno);
            return -1;
        }
    }
    return 0;
}

static int uvc_v4l2_enqueue(struct uvc_ctx *uvc, void *buf, size_t len)
{
    struct v4l2_ctx *c = (struct v4l2_ctx *)uvc->opaque;
    struct v4l2_buffer qbuf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = c->buf_index
    };

    if (c->qbuf_done) {
        return 0;
    }
    if (v4l2_ioctl(c->fd, VIDIOC_QBUF, &qbuf) < 0) {
        printf("%s ioctl(VIDIOC_QBUF) failed: %d\n", __func__, errno);
        return -1;
    }
    c->qbuf_done = true;
    return 0;
}

static int uvc_v4l2_dequeue(struct uvc_ctx *uvc, struct video_frame *frame)
{
    int retry_cnt = 0;
    uint8_t *start;
    struct v4l2_buffer qbuf;
    int i;

    struct v4l2_ctx *c = (struct v4l2_ctx *)uvc->opaque;
    if (!c->qbuf_done) {
        printf("v4l2 need VIDIOC_QBUF first!\n");
        return -1;
    }

    memset(&qbuf, 0, sizeof(qbuf));
    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qbuf.memory = V4L2_MEMORY_MMAP;

retry:
    if (v4l2_ioctl(c->fd, VIDIOC_DQBUF, &qbuf) < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            if (++retry_cnt > MAX_V4L2_DQBUF_RETYR_CNT) {
                return -1;
            }
            goto retry;
        } else {
            printf("%s ioctl(VIDIOC_DQBUF) failed: %d\n", __func__, errno);
            return -1;
        }
    }

    c->qbuf_done = false;
    c->buf_index = qbuf.index;

    frame->timestamp = timeval2ns(qbuf.timestamp);
    if (c->frame_id == 0) {
        c->first_ts = frame->timestamp;
    }
    frame->timestamp -= c->first_ts;
    frame->id = c->frame_id;
    c->frame_id++;

    c->prev_ts = frame->timestamp;
    start = (uint8_t *)c->buf[qbuf.index].iov_base;

    if (frame->flag == VFC_NONE) {//frame data ptr
        for (i = 0; i < frame->planes; ++i) {
            frame->data[i] = start + frame->plane_offsets[i];
        }
    } else {//frame data copy
        for (i = 0; i < frame->planes; ++i) {
            memcpy(frame->data[i], start + frame->plane_offsets[i],
                   frame->linesize[i]*frame->height);
        }
    }
    frame->total_size = qbuf.bytesused;

    return frame->total_size;
}

static void uvc_v4l2_close(struct uvc_ctx *uvc)
{
    struct v4l2_ctx *c = (struct v4l2_ctx *)uvc->opaque;
    uvc_v4l2_stop_stream(uvc);
    free(c->name);
    v4l2_close(c->fd);
    free(c);
}

static int v4l2_get_input(struct v4l2_ctx *c)
{
    struct v4l2_input input;
    struct v4l2_standard standard;
    v4l2_std_id std_id;

    memset(&input, 0, sizeof(input));

    if (v4l2_ioctl(c->fd, VIDIOC_G_INPUT, &input.index) < 0) {
        printf("ioctl VIDIOC_G_INPUT failed\n");
        return -1;
    }
    if (v4l2_ioctl(c->fd, VIDIOC_ENUMINPUT, &input) < 0) {
        printf("Unable to query input %d VIDIOC_ENUMINPUT,"
               "if you use a WEBCAM change input value in conf by -1",
              input.index);
        return -1;
    }
    c->channel = input.index;

    printf("[V4L2 Input information]:\n");
    printf("\tinput.name: \t\"%s\"\n", input.name);
    if (0 == v4l2_ioctl(c->fd, VIDIOC_G_STD, &std_id)) {
        memset(&standard, 0, sizeof(standard));
        standard.index = 0;
        while (v4l2_ioctl(c->fd, VIDIOC_ENUMSTD, &standard) == 0) {
            if (standard.id & std_id)
                printf("\t\t- video standard %s\n", standard.name);
            standard.index++;
        }
        printf("Set standard method %d\n", (int)std_id);
    }
    return 0;
}

static int v4l2_get_cap(struct v4l2_ctx *c)
{
    if (v4l2_ioctl(c->fd, VIDIOC_QUERYCAP, &c->cap) < 0) {
        printf("failed to VIDIOC_QUERYCAP\n");
        return -1;
    }

    printf("[V4L2 Capability Information]:\n");
    printf("\tcap.driver: \t\"%s\"\n", c->cap.driver);
    printf("\tcap.card: \t\"%s\"\n", c->cap.card);
    printf("\tcap.bus_info: \t\"%s\"\n", c->cap.bus_info);
    printf("\tcap.version: \t\"%d\"\n", c->cap.version);
    printf("\tcap.capabilities:\n");
    if (c->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        printf("\t\t\t VIDEO_CAPTURE\n");
    if (c->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        printf("\t\t\t VIDEO_OUTPUT\n");
    if (c->cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
        printf("\t\t\t VIDEO_OVERLAY\n");
    if (c->cap.capabilities & V4L2_CAP_VBI_CAPTURE)
        printf("\t\t\t VBI_CAPTURE\n");
    if (c->cap.capabilities & V4L2_CAP_VBI_OUTPUT)
        printf("\t\t\t VBI_OUTPUT\n");
    if (c->cap.capabilities & V4L2_CAP_RDS_CAPTURE)
        printf("\t\t\t RDS_CAPTURE\n");
    if (c->cap.capabilities & V4L2_CAP_TUNER)
        printf("\t\t\t TUNER\n");
    if (c->cap.capabilities & V4L2_CAP_AUDIO)
        printf("\t\t\t AUDIO\n");
    if (c->cap.capabilities & V4L2_CAP_READWRITE)
        printf("\t\t\t READWRITE\n");
    if (c->cap.capabilities & V4L2_CAP_ASYNCIO)
        printf("\t\t\t ASYNCIO\n");
    if (c->cap.capabilities & V4L2_CAP_STREAMING)
        printf("\t\t\t STREAMING\n");
    if (c->cap.capabilities & V4L2_CAP_TIMEPERFRAME)
        printf("\t\t\t TIMEPERFRAME\n");
    if (!(c->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        printf("Device does not support capturing.\n");
        return -1;
    }
    return 0;
}

static void v4l2_get_fmt(struct v4l2_ctx *c)
{
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("[V4L2 Support Format]:\n");
    while (0 == v4l2_ioctl(c->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        printf("\t%d. [%s] \"%s\"\n", fmtdesc.index,
               V4L2_FOURCC_STR(fmtdesc.pixelformat),
               fmtdesc.description);
        ++fmtdesc.index;
    }
}

static void v4l2_get_cid(struct v4l2_ctx *c)
{
    int i;
    struct v4l2_queryctrl *qctrl;
    struct v4l2_control control;
    printf("[V4L2 Supported Control]:\n");
    for (i = 0; v4l2_cid_supported[i] != V4L2_CID_LASTP1; i++) {
        qctrl = &c->controls[i];
        qctrl->id = v4l2_cid_supported[i];
        if (v4l2_ioctl(c->fd, VIDIOC_QUERYCTRL, qctrl) < 0) {
            continue;
        }
        c->ctrl_flags |= 1 << i;
        memset(&control, 0, sizeof (control));
        control.id = v4l2_cid_supported[i];
        if (v4l2_ioctl(c->fd, VIDIOC_G_CTRL, &control) < 0) {
            continue;
        }
        printf("\t%s, range: [%d, %d], default: %d, current: %d\n",
             qctrl->name,
             qctrl->minimum, qctrl->maximum,
             qctrl->default_value, control.value);
    }
}

static int uvc_v4l2_print_info(struct v4l2_ctx *c)
{
    v4l2_get_input(c);
    v4l2_get_cap(c);
    v4l2_get_fmt(c);
    v4l2_get_cid(c);
    return 0;
}

static int uvc_v4l2_ioctl(struct uvc_ctx *uvc, unsigned long int cmd, ...)
{
    void *arg;
    va_list ap;
    int ret = -1;
    struct v4l2_ctx *c = (struct v4l2_ctx *)uvc->opaque;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case UVC_GET_CAP:
        uvc_v4l2_print_info(c);
        break;
    default:
        ret = v4l2_ioctl(c->fd, cmd, arg);
        break;
    }

    return ret;
}

struct uvc_ops v4l2_ops = {
    .open         = uvc_v4l2_open,
    .close        = uvc_v4l2_close,
    .dequeue      = uvc_v4l2_dequeue,
    .enqueue      = uvc_v4l2_enqueue,
    .ioctl        = uvc_v4l2_ioctl,
    .start_stream = uvc_v4l2_start_stream,
    .stop_stream  = uvc_v4l2_stop_stream,
};
