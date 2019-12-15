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
    int resolution;
    int pixfmt;
    int linesize;
    int dv_timing;
    int framerate;
    char *name;
    int width;
    int height;
    struct iovec buf[MAX_V4L_BUF];
    int buf_index;
    int req_count;
    bool qbuf_done;
    uint64_t first_ts;
    uint64_t prev_ts;
    uint64_t got_frame_cnt;
    struct v4l2_capability cap;
    uint32_t ctrl_flags;
    struct v4l2_queryctrl controls[MAX_V4L2_CID];
    struct uvc_ctx *parent;
};

static int v4l2_init(struct v4l2_ctx *vc);
static int v4l2_create_mmap(struct v4l2_ctx *vc);
static int v4l2_set_format(int fd, int *resolution, int *pixelformat, int *bytesperline);
static int v4l2_set_framerate(int fd, int *framerate);
static int uvc_v4l2_start_stream(struct uvc_ctx *uvc);

static inline int v4l2_pack_tuple(int a, int b)
{
    return (a << 16) | (b & 0xffff);
}

static void v4l2_unpack_tuple(int *a, int *b, int packed)
{
    *a = packed >> 16;
    *b = packed & 0xffff;
}

#define V4L2_FOURCC_STR(code)                                         \
        (char[5])                                                     \
{                                                                     \
    (code >> 24) & 0xFF, (code >> 16) & 0xFF, (code >> 8) & 0xFF,     \
    code & 0xFF, 0                                                    \
}

#define timeval2ns(tv) \
    (((uint64_t)tv.tv_sec * 1000000000) + ((uint64_t)tv.tv_usec * 1000))


static void *uvc_v4l2_open(struct uvc_ctx *uvc, const char *dev, int width, int height)
{
    int fd = -1;
    int fps_num, fps_denom;
    struct v4l2_ctx *vc = calloc(1, sizeof(struct v4l2_ctx));
    if (!vc) {
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
    vc->name = strdup(dev);
    vc->fd = fd;
    vc->width = width;
    vc->height = height;
    vc->channel = -1;
    vc->standard = -1;
    vc->resolution = -1;
    vc->framerate = -1;
    vc->dv_timing = -1;

    if (-1 == v4l2_init(vc)) {
        printf("v4l2_init failed\n");
        goto failed;
    }
    if (-1 == v4l2_set_format(vc->fd, &vc->resolution, &vc->pixfmt, &vc->linesize)) {
        printf("v4l2_set_format failed\n");
        goto failed;
    }

    v4l2_unpack_tuple(&vc->width, &vc->height, vc->resolution);

    /* set framerate */
    if (v4l2_set_framerate(vc->fd, &vc->framerate) < 0) {
        printf("Unable to set framerate\n");
        goto failed;
    }
    v4l2_unpack_tuple(&fps_num, &fps_denom, vc->framerate);
    printf("Resolution: %dx%d\n", vc->width, vc->height);
    printf("Pixelformat: %s\n", V4L2_FOURCC_STR(vc->pixfmt));
    printf("Linesize: %d Bytes\n", vc->linesize);
    printf("Framerate: %.2f fps\n", (float)fps_denom / fps_num);

    if (-1 == v4l2_create_mmap(vc)) {
        printf("v4l2_create_mmap failed\n");
        goto failed;
    }

    uvc->fd = fd;
    vc->parent = uvc;
    return vc;

failed:
    if (fd != -1) {
        v4l2_close(fd);
    }
    if (vc->name) {
        free(vc->name);
    }
    if (vc) {
        free(vc);
    }
    return NULL;
}


static int v4l2_get_cap(struct v4l2_ctx *vc)
{
    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_QUERYCAP, &vc->cap)) {
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
    while (-1 != v4l2_ioctl(vc->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        printf("\t%d. [%s] \"%s\"\n", fmtdesc.index,
               v4l2_fourcc_parse(pixel, sizeof(pixel), fmtdesc.pixelformat),
               fmtdesc.description);
        ++fmtdesc.index;
    }
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

static int v4l2_init(struct v4l2_ctx *vc)
{
    uint32_t input_caps = 0;
    struct v4l2_input in;
    memset(&in, 0, sizeof(in));

    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_G_INPUT, &in.index)) {
        printf("ioctl VIDIOC_G_INPUT failed\n");
        return -1;
    }

    if (v4l2_ioctl(vc->fd, VIDIOC_ENUMINPUT, &in) < 0) {
        printf("ioctl VIDIOC_ENUMINPUT failed\n");
        return -1;
    }

    input_caps = in.capabilities;

    if (input_caps & V4L2_IN_CAP_STD) {
        if (v4l2_set_standard(vc->fd, &vc->standard) < 0) {
            printf("Unable to set video standard\n");
            return -1;
        }
        vc->resolution = -1;
        vc->framerate = -1;
    }

    if (input_caps & V4L2_IN_CAP_DV_TIMINGS) {
        if (v4l2_set_dv_timing(vc->fd, &vc->dv_timing) < 0) {
            printf("Unable to set dv timing\n");
            return -1;
        }
        vc->resolution = -1;
        vc->framerate = -1;
    }
    return 0;
}

static int v4l2_get_input(struct v4l2_ctx *vc)
{
    struct v4l2_input input;
    struct v4l2_standard standard;
    v4l2_std_id std_id;

    memset(&input, 0, sizeof(input));

    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_G_INPUT, &input.index)) {
        printf("ioctl VIDIOC_G_INPUT failed\n");
        return -1;
    }
    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_ENUMINPUT, &input)) {
        printf("Unable to query input %d VIDIOC_ENUMINPUT,"
               "if you use a WEBCAM change input value in conf by -1",
              input.index);
        return -1;
    }
    vc->channel = input.index;

    printf("[V4L2 Input information]:\n");
    printf("\tinput.name: \t\"%s\"\n", input.name);
    if (input.type & V4L2_INPUT_TYPE_TUNER)
        printf("\t\t\t TUNER\n");
    if (input.type & V4L2_INPUT_TYPE_CAMERA)
        printf("\t\t\t CAMERA\n");
    if (-1 != v4l2_ioctl(vc->fd, VIDIOC_G_STD, &std_id)) {
        memset(&standard, 0, sizeof(standard));
        standard.index = 0;
        while (v4l2_ioctl(vc->fd, VIDIOC_ENUMSTD, &standard) == 0) {
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
        if (-1 == v4l2_ioctl(vc->fd, VIDIOC_QUERYCTRL, qctrl)) {
            continue;
        }
        vc->ctrl_flags |= 1 << i;
        memset(&control, 0, sizeof (control));
        control.id = v4l2_cid_supported[i];
        if (-1 == v4l2_ioctl(vc->fd, VIDIOC_G_CTRL, &control)) {
            continue;
        }
        printf("\t%s, range: [%d, %d], default: %d, current: %d\n",
             qctrl->name,
             qctrl->minimum, qctrl->maximum,
             qctrl->default_value, control.value);
    }
}

static int v4l2_set_format(int fd, int *resolution,
                int *pixelformat, int *bytesperline)
{
    bool set = false;
    int width, height;
    struct v4l2_format fmt;

    /* We need to set the type in order to query the settings */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        printf("%s ioctl(VIDIOC_G_FMT) failed: %d\n", __func__, errno);
        return -1;
    }

    if (*resolution != -1) {
        v4l2_unpack_tuple(&width, &height, *resolution);
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        set = true;
    }

    if (*pixelformat != -1) {
        fmt.fmt.pix.pixelformat = *pixelformat;
        set = true;
    }

    if (set && (v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0))
        return -1;

    *resolution = v4l2_pack_tuple(fmt.fmt.pix.width, fmt.fmt.pix.height);
    *pixelformat = fmt.fmt.pix.pixelformat;
    *bytesperline = fmt.fmt.pix.bytesperline;
    return 0;
}

static int v4l2_set_framerate(int fd, int *framerate)
{
    bool set = false;
    int num, denom;
    struct v4l2_streamparm par;

    /* We need to set the type in order to query the stream settings */
    par.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_G_PARM, &par) < 0)
        return -1;

    if (*framerate != -1) {
        v4l2_unpack_tuple(&num, &denom, *framerate);
        par.parm.capture.timeperframe.numerator = num;
        par.parm.capture.timeperframe.denominator = denom;
        set = true;
    }

    if (set && (v4l2_ioctl(fd, VIDIOC_S_PARM, &par) < 0))
        return -1;

    *framerate = v4l2_pack_tuple(par.parm.capture.timeperframe.numerator,
                    par.parm.capture.timeperframe.denominator);
    return 0;
}

static int uvc_v4l2_start_stream(struct uvc_ctx *uvc)
{
    enum v4l2_buf_type type;
    struct v4l2_buffer enq;
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;

    memset(&enq, 0, sizeof(enq));
    enq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enq.memory = V4L2_MEMORY_MMAP;

    for (enq.index = 0; enq.index < vc->req_count; ++enq.index) {
        if (v4l2_ioctl(vc->fd, VIDIOC_QBUF, &enq) < 0) {
            printf("unable to queue buffer\n");
            return -1;
        }
    }
    vc->qbuf_done = true;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(vc->fd, VIDIOC_STREAMON, &type) < 0) {
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
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;

    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_STREAMOFF, &type)) {
        printf("%s ioctl(VIDIOC_STREAMOFF) failed: %d\n", __func__, errno);
    }
    for (i = 0; i < vc->req_count; i++) {
        v4l2_munmap(vc->buf[i].iov_base, vc->buf[i].iov_len);
    }
    return 0;
}

static int v4l2_create_mmap(struct v4l2_ctx *vc)
{
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers req = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .count = MAX_V4L_REQBUF_CNT,
        .memory = V4L2_MEMORY_MMAP
    };
    //request buffer
    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_REQBUFS, &req)) {
        printf("%s ioctl(VIDIOC_REQBUFS) failed: %d\n", __func__, errno);
        return -1;
    }
    vc->req_count = req.count;
    if (req.count > MAX_V4L_REQBUF_CNT || req.count < 2) {
        printf("Insufficient buffer memory\n");
        return -1;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = req.type;
    buf.memory = req.memory;

    for (buf.index = 0; buf.index < vc->req_count; ++buf.index) {
        //query buffer
        if (-1 == v4l2_ioctl(vc->fd, VIDIOC_QUERYBUF, &buf)) {
            printf("%s ioctl(VIDIOC_QUERYBUF) failed: %d\n", __func__, errno);
            return -1;
        }
        //mmap buffer
        vc->buf[buf.index].iov_len = buf.length;
        vc->buf[buf.index].iov_base =
                v4l2_mmap(NULL, buf.length, PROT_READ|PROT_WRITE,
                                MAP_SHARED, vc->fd, buf.m.offset);
        if (MAP_FAILED == vc->buf[buf.index].iov_base) {
            printf("mmap failed: %d\n", errno);
            return -1;
        }
    }
    return 0;
}

static void v4l2_prepare_frame(struct v4l2_ctx *vc,
                struct uvc_frame *frame, size_t *plane_offsets)
{
    memset(frame, 0, sizeof(struct uvc_frame));
    memset(plane_offsets, 0, sizeof(size_t) * MAX_AV_PLANES);

    frame->width = vc->width;
    frame->height = vc->height;

    switch (vc->pixfmt) {
    case V4L2_PIX_FMT_NV12:
        frame->linesize[0] = vc->linesize;
        frame->linesize[1] = vc->linesize / 2;
        plane_offsets[1] = vc->linesize * vc->height;
        break;
    case V4L2_PIX_FMT_YVU420:
        frame->linesize[0] = vc->linesize;
        frame->linesize[1] = vc->linesize / 2;
        frame->linesize[2] = vc->linesize / 2;
        plane_offsets[1] = vc->linesize * vc->height * 5 / 4;
        plane_offsets[2] = vc->linesize * vc->height;
        break;
    case V4L2_PIX_FMT_YUV420:
        frame->linesize[0] = vc->linesize;
        frame->linesize[1] = vc->linesize / 2;
        frame->linesize[2] = vc->linesize / 2;
        plane_offsets[1] = vc->linesize * vc->height;
        plane_offsets[2] = vc->linesize * vc->height * 5 / 4;
        break;
    case V4L2_PIX_FMT_UYVY:
    default:
        frame->linesize[0] = vc->linesize;
        break;
    }
}

static int uvc_v4l2_enqueue(struct uvc_ctx *uvc, void *buf, size_t len)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;
    struct v4l2_buffer qbuf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = vc->buf_index
    };

    if (vc->qbuf_done) {
        return 0;
    }
    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_QBUF, &qbuf)) {
        printf("%s ioctl(VIDIOC_QBUF) failed: %d\n", __func__, errno);
        return -1;
    }
    vc->qbuf_done = true;
    return 0;
}

static int uvc_v4l2_dequeue(struct uvc_ctx *uvc, struct uvc_frame *frame)
{
    int retry_cnt = 0;
    uint8_t *start;
    size_t plane_offsets[MAX_AV_PLANES];
    struct v4l2_buffer qbuf;

    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;
    if (!vc->qbuf_done) {
        printf("v4l2 need VIDIOC_QBUF first!\n");
        return -1;
    }

    memset(&qbuf, 0, sizeof(qbuf));
    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qbuf.memory = V4L2_MEMORY_MMAP;

retry:
    if (-1 == v4l2_ioctl(vc->fd, VIDIOC_DQBUF, &qbuf)) {
        printf("%s ioctl(VIDIOC_DQBUF) failed: %d\n", __func__, errno);
        if (errno == EINTR || errno == EAGAIN) {
            if (++retry_cnt > MAX_V4L2_DQBUF_RETYR_CNT) {
                return -1;
            }
            goto retry;
        } else {
                return -1;
        }
    }

    vc->qbuf_done = false;
    vc->buf_index = qbuf.index;

    v4l2_prepare_frame(vc, frame, plane_offsets);
    frame->timestamp = timeval2ns(qbuf.timestamp);
    if (!vc->got_frame_cnt) {
        vc->first_ts = frame->timestamp;
    }
    vc->got_frame_cnt++;
    frame->timestamp -= vc->first_ts;
    frame->id = vc->got_frame_cnt;
    //printf("frame id=%zu, timestamp=%zu, gap=%zu\n", frame->id, frame->timestamp, frame->timestamp-vc->prev_ts);

    vc->prev_ts = frame->timestamp;
    start = (uint8_t *)vc->buf[qbuf.index].iov_base;
    for (int i = 0; i < MAX_AV_PLANES; ++i) {
        frame->data[i] = start + plane_offsets[i];
    }
    frame->size = qbuf.bytesused;

    return frame->size;
}

static void uvc_v4l2_close(struct uvc_ctx *uvc)
{
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;
    uvc_v4l2_stop_stream(uvc);
    free(vc->name);
    v4l2_close(vc->fd);
    free(vc);
}

static int uvc_v4l2_print_info(struct v4l2_ctx *vc)
{
    v4l2_get_input(vc);
    v4l2_get_cap(vc);
    v4l2_get_fmt(vc);
    v4l2_get_cid(vc);
    return 0;
}


static int uvc_v4l2_ioctl(struct uvc_ctx *uvc, unsigned long int cmd, ...)
{
    void *arg;
    va_list ap;
    int ret = -1;
    struct v4l2_ctx *vc = (struct v4l2_ctx *)uvc->opaque;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case UVC_GET_CAP:
        uvc_v4l2_print_info(vc);
        break;
    default:
        ret = v4l2_ioctl(vc->fd, cmd, arg);
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
