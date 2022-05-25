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
#include <libthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
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
#if HAVE_LIBV4L2
void *(*mmap_f)(void *start, size_t length, int prot, int flags, int fd, int64_t offset);
#else
void *(*mmap_f)(void *start, size_t length, int prot, int flags, int fd, off_t offset);
#endif
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
    V4L2_CID_SHARPNESS,
    V4L2_CID_LASTP1
};

#define MAX_V4L2_CID             (sizeof(v4l2_cid_supported)/sizeof(uint32_t))
#define MAX_V4L_BUF              (32)
#define MAX_V4L_REQBUF_CNT       (4)
#define MAX_V4L2_DQBUF_RETYR_CNT (5)

struct v4l2_ctx {
    int fd;
    int cancel_fd;
    int channel; /*one video node may contain several input channel */
    int standard;
    uint32_t pixfmt;
    uint32_t linesize;
    int dv_timing;
    uint32_t width;
    uint32_t height;
    uint32_t fps_num;
    uint32_t fps_den;
    struct iovec buf[MAX_V4L_BUF];
    int buf_index;
    int req_count;
    bool qbuf_done;
    uint64_t first_ts;
    uint64_t frame_id;
    struct v4l2_queryctrl controls[MAX_V4L2_CID];
    struct avcap_ctx *parent;
    struct thread *thread;
    bool is_streaming;
    int epfd;
    struct epoll_event events;
};

static int avcap_v4l2_init(struct v4l2_ctx *c);
static int avcap_v4l2_create_mmap(struct v4l2_ctx *c);
static int avcap_v4l2_set_format(int fd, uint32_t *w, uint32_t *h, uint32_t *pixelformat, uint32_t *bytesperline);
static int avcap_v4l2_set_framerate(int fd, uint32_t *fps_num, uint32_t *fps_den);
//static int _v4l2_start_stream(struct avcap_ctx *avcap);


#define V4L2_FOURCC_STR(code)                                               \
        (char[5])                                                           \
{                                                                           \
    (code&0xFF), ((code>>8)&0xFF), ((code>>16)&0xFF), ((code>>24)&0xFF), 0  \
}

#define timeval2ns(tv) \
    (((uint64_t)tv.tv_sec * 1000000000) + ((uint64_t)tv.tv_usec * 1000))

static const struct v4l2_fourcc_pixel_format {
    enum pixel_format pxl_fmt;
    uint32_t v4l2_fmt;
} v4l2_fourcc_pixel_format_map [] = {
    //pixel_format          v4l2_format
    {PIXEL_FORMAT_I420,     V4L2_PIX_FMT_YVU420},
    {PIXEL_FORMAT_I420,     V4L2_PIX_FMT_YUV420},
    {PIXEL_FORMAT_NV12,     V4L2_PIX_FMT_NV12},
    {PIXEL_FORMAT_YVYU,     V4L2_PIX_FMT_YVYU},
    {PIXEL_FORMAT_YUY2,     V4L2_PIX_FMT_YUYV},
    {PIXEL_FORMAT_YUY2,     V4L2_PIX_FMT_VYUY},
    {PIXEL_FORMAT_UYVY,     V4L2_PIX_FMT_UYVY},
    {PIXEL_FORMAT_RGBA,     0},
    {PIXEL_FORMAT_BGRA,     V4L2_PIX_FMT_ABGR32},
    {PIXEL_FORMAT_BGRX,     V4L2_PIX_FMT_XBGR32},
    {PIXEL_FORMAT_Y800,     0},
    {PIXEL_FORMAT_I444,     V4L2_PIX_FMT_YUV444},
    {PIXEL_FORMAT_BGR3,     V4L2_PIX_FMT_BGR24},
    {PIXEL_FORMAT_I422,     0},
    {PIXEL_FORMAT_I40A,     0},
    {PIXEL_FORMAT_I42A,     0},
    {PIXEL_FORMAT_YUVA,     0},
    {PIXEL_FORMAT_AYUV,     0},
    {PIXEL_FORMAT_JPEG,     V4L2_PIX_FMT_JPEG},
    {PIXEL_FORMAT_MJPG,     V4L2_PIX_FMT_MJPEG},
    {PIXEL_FORMAT_MJPG,     V4L2_PIX_FMT_SBGGR10P},
    {PIXEL_FORMAT_MAX,      0},
};

static enum pixel_format v4l2fmt_to_pxlfmt(uint32_t v4l2_fmt)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(v4l2_fourcc_pixel_format_map); i++) {
        if (v4l2_fourcc_pixel_format_map[i].v4l2_fmt == v4l2_fmt)
            return v4l2_fourcc_pixel_format_map[i].pxl_fmt;
    }
    printf("v4l2fmt_to_pxlfmt %s failed!\n", V4L2_FOURCC_STR(v4l2_fmt));
    return PIXEL_FORMAT_NONE;
}

static uint32_t pxlfmt_to_v4l2fmt(enum pixel_format pxl_fmt)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(v4l2_fourcc_pixel_format_map); i++) {
        if (v4l2_fourcc_pixel_format_map[i].pxl_fmt == pxl_fmt)
            return v4l2_fourcc_pixel_format_map[i].v4l2_fmt;
    }
    printf("pxlfmt_to_v4l2fmt %s failed!\n", pixel_format_to_string(pxl_fmt));
    return 0;
}

static void *_v4l2_open(struct avcap_ctx *avcap, const char *dev, struct avcap_config *avconf)
{
    int fd = -1;
    struct videocap_config *conf = &avconf->video;
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
    c->fd = fd;
    c->cancel_fd = eventfd(0, 0);

    c->channel = -1;
    c->standard = -1;
    c->dv_timing = -1;
    c->width = conf->width;
    c->height = conf->height;
    c->pixfmt = pxlfmt_to_v4l2fmt(conf->format);
    c->fps_num = conf->fps.num;
    c->fps_den = conf->fps.den;
    c->frame_id = 0;

    if (-1 == avcap_v4l2_init(c)) {
        printf("avcap_v4l2_init failed\n");
        goto failed;
    }

    if (avcap_v4l2_set_format(c->fd, &c->width, &c->height, &c->pixfmt, &c->linesize) < 0) {
        printf("%s:%d avcap_v4l2_set_format failed %d\n", __func__, __LINE__, errno);
        goto failed;
    }

    if (avcap_v4l2_set_framerate(c->fd, &c->fps_num, &c->fps_den) < 0) {
        printf("avcap_v4l2_set_framerate failed\n");
        //goto failed;
    }

    if (avcap_v4l2_create_mmap(c) < 0) {
        printf("avcap_v4l2_create_mmap failed\n");
        goto failed;
    }

    avcap->conf.video.width = c->width;
    avcap->conf.video.height = c->height;
    avcap->conf.video.fps.num = c->fps_num;
    avcap->conf.video.fps.den = c->fps_den;
    avcap->conf.video.format = v4l2fmt_to_pxlfmt(c->pixfmt);
    avcap->fd = fd;
    c->parent = avcap;
    return c;

failed:
    if (fd != -1) {
        v4l2_close(fd);
    }
    if (c) {
        free(c);
    }
    return NULL;
}

static int avcap_v4l2_set_config(struct avcap_ctx *avcap, struct avcap_config *avconf)
{
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;
    struct videocap_config *conf = &avconf->video;

    c->width   = conf->width;
    c->height  = conf->height;
    c->fps_num = conf->fps.num;
    c->fps_den = conf->fps.den;
    c->pixfmt  = pxlfmt_to_v4l2fmt(conf->format);

    if (avcap_v4l2_set_format(c->fd, &c->width, &c->height, &c->pixfmt, &c->linesize) < 0) {
        printf("%s:%d avcap_v4l2_set_format failed %d\n", __func__, __LINE__, errno);
        return -1;
    }

    if (avcap_v4l2_set_framerate(c->fd, &c->fps_num, &c->fps_den) < 0) {
        printf("avcap_v4l2_set_framerate failed\n");
        return -1;
    }

    avcap->conf.video.width = c->width;
    avcap->conf.video.height = c->height;
    avcap->conf.video.fps.num = c->fps_num;
    avcap->conf.video.fps.den = c->fps_den;
    avcap->conf.video.format = v4l2fmt_to_pxlfmt(c->pixfmt);
    return 0;
}

static int v4l2_set_standard(int fd, int *standard)
{
    if (*standard == -1) {
        if (v4l2_ioctl(fd, VIDIOC_G_STD, standard) < 0) {
            printf("%s VIDIOC_G_STD failed: %d\n", __func__, errno);
            return -1;
        }
    } else {
        if (v4l2_ioctl(fd, VIDIOC_S_STD, standard) < 0) {
            printf("%s VIDIOC_S_STD failed: %d\n", __func__, errno);
            return -1;
        }
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

    if (v4l2_ioctl(dev, VIDIOC_ENUM_DV_TIMINGS, &iter) < 0) {
        printf("%s VIDIOC_ENUM_DV_TIMINGS failed: %d\n", __func__, errno);
        return -1;
    }

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

    if (v4l2_ioctl(fd, VIDIOC_S_DV_TIMINGS, &dvt) < 0) {
        printf("%s VIDIOC_S_DV_TIMINGS failed: %d\n", __func__, errno);
        return -1;
    }

    return 0;
}

static int v4l2_get_control(int fd, uint32_t id, int *val)
{
    struct v4l2_control vctrl;

    vctrl.id = id;
    if (v4l2_ioctl(fd, VIDIOC_G_CTRL, &vctrl) < 0) {
        printf("%s VIDIOC_G_CTRL failed: %d\n", __func__, errno);
        return -1;
    }
    *val = vctrl.value;
    return 0;
}

static int v4l2_set_control(int fd, uint32_t id, int val)
{
    struct v4l2_control vctrl;

    vctrl.id = id;
    vctrl.value = val;
    if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &vctrl) < 0) {
        printf("%s VIDIOC_S_CTRL failed: %d\n", __func__, errno);
        return -1;
    }
    return 0;
}

static int avcap_v4l2_init(struct v4l2_ctx *c)
{
    struct v4l2_input in;
    memset(&in, 0, sizeof(in));

    if (v4l2_ioctl(c->fd, VIDIOC_G_INPUT, &in.index) < 0) {
        printf("ioctl VIDIOC_G_INPUT failed:%d\n", errno);
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

static int avcap_v4l2_set_format(int fd, uint32_t *width, uint32_t *height,
                uint32_t *pixelformat, uint32_t *bytesperline)
{
    bool update;
    struct v4l2_format fmt;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        printf("%s VIDIOC_G_FMT failed: %d\n", __func__, errno);
        return -1;
    }

    if (fmt.fmt.pix.width == *width && fmt.fmt.pix.height == *height &&
        fmt.fmt.pix.pixelformat == *pixelformat) {
        update = false;
    } else {
        update = true;
        fmt.fmt.pix.width       = *width;
        fmt.fmt.pix.height      = *height;
        fmt.fmt.pix.pixelformat = *pixelformat;
    }

    if (update && v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        printf("%s VIDIOC_S_FMT failed: %d\n", __func__, errno);
        return -1;
    }

    if (fmt.fmt.pix.width != *width || fmt.fmt.pix.height != *height) {
        printf("v4l2 resolution force from %d*%d to %d*%d\n",
                *width, *height, fmt.fmt.pix.width,fmt.fmt.pix.height);
    }

    if (fmt.fmt.pix.pixelformat != *pixelformat) {
        printf("v4l2 format force from %s to %s\n",
                V4L2_FOURCC_STR(*pixelformat),
                V4L2_FOURCC_STR(fmt.fmt.pix.pixelformat));
    }

    *width = fmt.fmt.pix.width;
    *height = fmt.fmt.pix.height;
    *pixelformat = fmt.fmt.pix.pixelformat;
    *bytesperline = fmt.fmt.pix.bytesperline;
    return 0;
}

static int avcap_v4l2_set_framerate(int fd, uint32_t *fps_num, uint32_t *fps_den)
{
    bool update;
    struct v4l2_streamparm par;

    par.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_G_PARM, &par) < 0) {
        printf("%s VIDIOC_G_PARM failed:%d\n", __func__, errno);
        return -1;
    }

    if (par.parm.capture.timeperframe.numerator == *fps_den &&
        par.parm.capture.timeperframe.denominator == *fps_num) {
        update = false;
    } else {
        update = true;
        par.parm.capture.timeperframe.numerator = *fps_den;
        par.parm.capture.timeperframe.denominator = *fps_num;
    }

    if (update && v4l2_ioctl(fd, VIDIOC_S_PARM, &par) < 0) {
        printf("%s VIDIOC_S_PARM failed:%d\n", __func__, errno);
        return -1;
    }

    if (par.parm.capture.timeperframe.numerator != *fps_den ||
        par.parm.capture.timeperframe.denominator != *fps_num) {
        printf("v4l2 framerate force from %d/%d to %d/%d\n",
                *fps_num, *fps_den,
                par.parm.capture.timeperframe.denominator,
                par.parm.capture.timeperframe.numerator);
    }
    *fps_den = par.parm.capture.timeperframe.numerator;
    *fps_num = par.parm.capture.timeperframe.denominator;
    return 0;
}

static int avcap_v4l2_enqueue(struct avcap_ctx *avcap, void *buf, size_t len)
{
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;
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

static int avcap_v4l2_dequeue(struct avcap_ctx *avcap, struct video_frame *frame)
{
    int retry_cnt = 0;
    uint8_t *start;
    struct v4l2_buffer qbuf;
    int i;

    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;
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
    frame->frame_id = c->frame_id;
    c->frame_id++;

    start = (uint8_t *)c->buf[qbuf.index].iov_base;

    if (frame->mem_type == MEDIA_MEM_SHALLOW) {//frame data ptr
        for (i = 0; i < frame->planes; ++i) {
            frame->data[i] = start + frame->plane_offsets[i];
        }
    } else if (frame->mem_type == MEDIA_MEM_DEEP) {//frame data copy
        switch (frame->format) {
        case PIXEL_FORMAT_YUY2:
            memcpy(frame->data[0], start + frame->plane_offsets[0], frame->linesize[0]*frame->height);
            break;
        case PIXEL_FORMAT_I420:
            memcpy(frame->data[0], start + frame->plane_offsets[0], frame->linesize[0]*frame->height);
            memcpy(frame->data[1], start + frame->plane_offsets[1], frame->linesize[1]*frame->height / 2);
            memcpy(frame->data[2], start + frame->plane_offsets[2], frame->linesize[2]*frame->height / 2);
            break;
        default:
            break;
        }
    }
    frame->total_size = qbuf.bytesused;

    return frame->total_size;
}

static int avcap_v4l2_poll_init(struct v4l2_ctx *c)
{
    struct epoll_event epev;
    memset(&epev, 0, sizeof(epev));
    c->epfd = epoll_create(1);
    if (c->epfd == -1) {
        printf("epoll_create failed %d!\n", errno);
        return -1;
    }

    epev.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLET | EPOLLERR;
    epev.data.ptr = (void *)c;
    if (-1 == epoll_ctl(c->epfd, EPOLL_CTL_ADD, c->fd, &epev)) {
        printf("epoll_ctl EPOLL_CTL_ADD failed %d!\n", errno);
        return -1;
    }
    if (-1 == epoll_ctl(c->epfd, EPOLL_CTL_ADD, c->cancel_fd, &epev)) {
        printf("epoll_ctl EPOLL_CTL_ADD failed %d!\n", errno);
        return -1;
    }
    return 0;
}

static int avcap_v4l2_poll(struct avcap_ctx *avcap, int timeout)
{
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;
    int ret = 0;

    int n = epoll_wait(c->epfd, &c->events, 1, timeout);
    switch (n) {
    case 0:
        printf("poll timeout\n");
        ret = -1;
        break;
    case -1:
        printf("poll error %d\n", errno);
        ret = -1;
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

static void avcap_v4l2_poll_deinit(struct v4l2_ctx *c)
{
    if (-1 == epoll_ctl(c->epfd, EPOLL_CTL_DEL, c->fd, NULL)) {
        printf("epoll_ctl EPOLL_CTL_DEL failed %d!\n", errno);
    }
    close(c->epfd);
}

static void *v4l2_thread(struct thread *t, void *arg)
{
    struct avcap_ctx *avcap = arg;
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;
    struct media_frame media;
    struct videocap_config *conf = &avcap->conf.video;

    if (avcap_v4l2_poll_init(c) == -1) {
        printf("avcap_v4l2_poll_init failed!\n");
    }
    media.type = MEDIA_TYPE_VIDEO;
    video_frame_init(&media.video, conf->format, conf->width, conf->height, MEDIA_MEM_SHALLOW);
    while (c->is_streaming) {
        if (avcap_v4l2_enqueue(avcap, NULL, 0) != 0) {
            printf("avcap_v4l2_enqueue failed\n");
            continue;
        }
        if (avcap_v4l2_poll(avcap, -1) != 0) {
            printf("avcap_v4l2_poll failed\n");
            continue;
        }

        if (avcap_v4l2_dequeue(avcap, &media.video) == -1) {
            printf("avcap_v4l2_dequeue failed\n");
        }
        avcap->on_media_frame(avcap, &media);
    }
    avcap_v4l2_poll_deinit(c);
    return NULL;
}

static int _v4l2_start_stream(struct avcap_ctx *avcap)
{
    enum v4l2_buf_type type;
    struct v4l2_buffer enq;
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;

    if (c->is_streaming) {
        printf("v4l2 is streaming already!\n");
        return -1;
    }
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

    c->is_streaming = true;
    if (avcap->on_media_frame) {
        c->thread = thread_create(v4l2_thread, avcap);
        if (!c->thread) {
            printf("thread_create v4l2_thread failed!\n");
            return -1;
        }
    }

    return 0;
}

static void avcap_v4l2_destroy_mmap(struct v4l2_ctx *c)
{
    for (int i = 0; i < c->req_count; ++i) {
        if (c->buf[i].iov_base != MAP_FAILED && c->buf[i].iov_base != 0)
            v4l2_munmap(c->buf[i].iov_base, c->buf[i].iov_len);
    }

    if (c->req_count) {
        c->req_count = 0;
    }
}

static int _v4l2_stop_stream(struct avcap_ctx *avcap)
{
    uint64_t notify = '1';
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;

    if (!c->is_streaming) {
        printf("v4l2 stream stopped already!\n");
        return -1;
    }

    if (avcap->on_media_frame) {
        c->is_streaming = false;
        if (sizeof(uint64_t) != write(c->cancel_fd, &notify, sizeof(uint64_t))) {
            perror("write error");
        }
        thread_join(c->thread);
        thread_destroy(c->thread);
    }

    if (v4l2_ioctl(c->fd, VIDIOC_STREAMOFF, &type) < 0) {
        printf("%s ioctl(VIDIOC_STREAMOFF) failed: %d\n", __func__, errno);
    }

    return 0;
}

static int _v4l2_query_frame(struct avcap_ctx *avcap, struct media_frame *frame)
{
    if (avcap_v4l2_enqueue(avcap, NULL, 0) != 0) {
        printf("avcap_v4l2_enqueue failed\n");
        return -1;
    }
    return avcap_v4l2_dequeue(avcap, &frame->video);
}

static int avcap_v4l2_create_mmap(struct v4l2_ctx *c)
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

static void _v4l2_close(struct avcap_ctx *avcap)
{
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;
    //_v4l2_stop_stream(avcap);
    avcap_v4l2_destroy_mmap(c);
    v4l2_close(c->fd);
    close(c->epfd);
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
    struct v4l2_capability cap;

    if (v4l2_ioctl(c->fd, VIDIOC_QUERYCAP, &cap) < 0) {
        printf("failed to VIDIOC_QUERYCAP\n");
        return -1;
    }

    printf("[V4L2 Capability Information]:\n");
    printf("\tcap.driver: \t\"%s\"\n", cap.driver);
    printf("\tcap.card: \t\"%s\"\n", cap.card);
    printf("\tcap.bus_info: \t\"%s\"\n", cap.bus_info);
    printf("\tcap.version: \t\"%d\"\n", cap.version);
    printf("\tcap.capabilities: 0x%x\n", cap.capabilities);
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        printf("\t\t\t VIDEO_CAPTURE\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        printf("\t\t\t VIDEO_OUTPUT\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
        printf("\t\t\t VIDEO_OVERLAY\n");
    if (cap.capabilities & V4L2_CAP_VBI_CAPTURE)
        printf("\t\t\t VBI_CAPTURE\n");
    if (cap.capabilities & V4L2_CAP_VBI_OUTPUT)
        printf("\t\t\t VBI_OUTPUT\n");
    if (cap.capabilities & V4L2_CAP_RDS_CAPTURE)
        printf("\t\t\t RDS_CAPTURE\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
        printf("\t\t\t VIDEO_OUTPUT_OVERLAY\n");
    if (cap.capabilities & V4L2_CAP_TUNER)
        printf("\t\t\t TUNER\n");
    if (cap.capabilities & V4L2_CAP_AUDIO)
        printf("\t\t\t AUDIO\n");
    if (cap.capabilities & V4L2_CAP_READWRITE)
        printf("\t\t\t READWRITE\n");
    if (cap.capabilities & V4L2_CAP_ASYNCIO)
        printf("\t\t\t ASYNCIO\n");
    if (cap.capabilities & V4L2_CAP_STREAMING)
        printf("\t\t\t STREAMING\n");
    if (cap.capabilities & V4L2_CAP_TIMEPERFRAME)
        printf("\t\t\t TIMEPERFRAME\n");
    if (cap.capabilities & V4L2_CAP_DEVICE_CAPS)
        printf("\t\t\t DEVICE_CAPS\n");
    if (cap.capabilities & V4L2_CAP_EXT_PIX_FORMAT)
        printf("\t\t\t EXT_PIX_FORMAT\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
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
    for (i = 0; i < MAX_V4L2_CID; i++) {
        qctrl = &c->controls[i];
        qctrl->id = v4l2_cid_supported[i];
        if (v4l2_ioctl(c->fd, VIDIOC_QUERYCTRL, qctrl) < 0) {
            continue;
        }
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

static int avcap_v4l2_print_info(struct v4l2_ctx *c)
{
    v4l2_get_input(c);
    v4l2_get_cap(c);
    v4l2_get_fmt(c);
    v4l2_get_cid(c);
    return 0;
}

static int _v4l2_ioctl(struct avcap_ctx *avcap, unsigned long int cmd, ...)
{
    void *arg;
    va_list ap;
    int ret = -1;
    struct v4l2_ctx *c = (struct v4l2_ctx *)avcap->opaque;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case VIDCAP_GET_CAP:
        ret = avcap_v4l2_print_info(c);
        break;
    case VIDCAP_SET_CONF:
        ret = avcap_v4l2_set_config(avcap, (struct avcap_config *)arg);
        break;
    case VIDCAP_SET_LUMA:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_BRIGHTNESS, *(int *)&arg);
        break;
    case VIDCAP_GET_LUMA:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_BRIGHTNESS, (int *)arg);
        break;
    case VIDCAP_SET_CTRST:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_CONTRAST, *(int *)&arg);
        break;
    case VIDCAP_GET_CTRST:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_CONTRAST, (int *)arg);
        break;
    case VIDCAP_SET_SAT:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_SATURATION, *(int *)&arg);
        break;
    case VIDCAP_GET_SAT:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_SATURATION, (int *)arg);
        break;
    case VIDCAP_SET_HUE:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_HUE, *(int *)&arg);
        break;
    case VIDCAP_GET_HUE:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_HUE, (int *)arg);
        break;
    case VIDCAP_SET_AWB:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_AUTO_WHITE_BALANCE, *(int *)&arg);
        break;
    case VIDCAP_GET_AWB:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_AUTO_WHITE_BALANCE, (int *)arg);
        break;
    case VIDCAP_SET_GAMMA:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_GAMMA, *(int *)&arg);
        break;
    case VIDCAP_GET_GAMMA:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_GAMMA, (int *)arg);
        break;
    case VIDCAP_SET_SHARP:
        ret = v4l2_set_control(avcap->fd, V4L2_CID_SHARPNESS, *(int *)&arg);
        break;
    case VIDCAP_GET_SHARP:
        ret = v4l2_get_control(avcap->fd, V4L2_CID_SHARPNESS, (int *)arg);
        break;
    default:
        printf("unsupport VIDCAP cmd 0x%lx\n", cmd);
        ret = -1;
        break;
    }
    return ret;
}

struct avcap_ops v4l2_ops = {
    ._open        = _v4l2_open,
    ._close       = _v4l2_close,
    .ioctl        = _v4l2_ioctl,
    .start_stream = _v4l2_start_stream,
    .stop_stream  = _v4l2_stop_stream,
    .query_frame  = _v4l2_query_frame,
};
