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
#include <libthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

struct dummy_ctx {
    int fd;
    int ev_fd;
    uint64_t seek_offset;
    uint64_t first_ts;
    uint64_t frame_id;
    struct uvc_ctx *parent;
    struct thread *thread;
    bool is_streaming;
    int epfd;
    struct epoll_event events;
};

#define timespec2ns(ts) \
    (((uint64_t)ts.tv_sec * 1000000000) + ((uint64_t)ts.tv_nsec))

static void *uvc_dummy_open(struct uvc_ctx *uvc, const char *dev, struct uvc_config *conf)
{
    struct dummy_ctx *c = calloc(1, sizeof(struct dummy_ctx));
    if (!c) {
        printf("malloc dummy_ctx failed!\n");
        return NULL;
    }

    c->fd = open(dev, O_RDWR);
    if (c->fd == -1) {
        printf("open %s failed: %d\n", dev, errno);
        goto failed;
    }
    c->frame_id = 0;
    c->seek_offset = 0;
    c->ev_fd = eventfd(0, 0);
    if (c->ev_fd == -1) {
        printf("eventfd failed %d\n", errno);
        goto failed;
    }
    printf("eventfd fd=%d\n", c->ev_fd);

    memcpy(&uvc->conf, conf, sizeof(struct uvc_config));
    c->parent = uvc;

    return c;

failed:
    if (c->fd != -1) {
        close(c->fd);
    }
    if (c) {
        free(c);
    }
    return NULL;
}

static int msleep(uint64_t ms)
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ms*1000;
    return select(0, NULL, NULL, NULL, &tv);
}

static int uvc_dummy_enqueue(struct uvc_ctx *uvc, void *buf, size_t len)
{
    uint64_t notify = '1';
    uint64_t delay_ms = 0;
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;

    delay_ms = (1000)/(uvc->conf.fps.num/(uvc->conf.fps.den*1.0));
    msleep(delay_ms);
    if (write(c->ev_fd, &notify, sizeof(uint64_t)) != sizeof(uint64_t)) {
        printf("%s failed to notify ev_fd=%d, %d\n", __func__, c->ev_fd, errno);
        return -1;
    }
    return 0;
}

static int uvc_dummy_dequeue(struct uvc_ctx *uvc, struct video_frame *frame)
{
    int i;
    ssize_t ret;
    size_t len;
    uint64_t notify;
    struct timespec ts;
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;

    if (read(c->ev_fd, &notify, sizeof(notify)) != sizeof(uint64_t)) {
        printf("failed to read from notify %d\n", errno);
        return -1;
    }

    for (i = 0; i < frame->planes; ++i) {
        len = frame->linesize[i]*frame->height;
        ret = read(c->fd, frame->data[i], len);
        if (ret != len) {
            printf("read failed: ret=%zd, len=%zu, errno=%d, ptr=%p\n", ret, len, errno, frame->data[i]);
            return -1;
        }
        c->seek_offset += len;
        lseek(c->fd, c->seek_offset, SEEK_SET);
    }
    if (-1 == clock_gettime(CLOCK_REALTIME, &ts)) {
        printf("clock_gettime failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
    frame->timestamp = timespec2ns(ts);
    if (c->frame_id == 0) {
        c->first_ts = frame->timestamp;
    }
    frame->timestamp -= c->first_ts;
    frame->frame_id = c->frame_id;
    c->frame_id++;

    return frame->total_size;
}

static int uvc_dummy_poll_init(struct dummy_ctx *c)
{
    struct epoll_event epev;
    memset(&epev, 0, sizeof(epev));
    printf("dummy_ctx=%p\n", c);
    c->epfd = epoll_create(1);
    if (c->epfd == -1) {
        printf("epoll_create failed %d!\n", errno);
        return -1;
    }

    epev.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLET | EPOLLERR;
    epev.data.ptr = (void *)c;
    if (-1 == epoll_ctl(c->epfd, EPOLL_CTL_ADD, c->ev_fd, &epev)) {
        printf("epoll_ctl EPOLL_CTL_ADD failed %d!\n", errno);
        return -1;
    }
    return 0;
}

static int uvc_dummy_poll(struct uvc_ctx *uvc, int timeout)
{
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;
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

static void uvc_dummy_poll_deinit(struct dummy_ctx *c)
{
    if (-1 == epoll_ctl(c->epfd, EPOLL_CTL_DEL, c->ev_fd, NULL)) {
        printf("epoll_ctl EPOLL_CTL_DEL failed %d!\n", errno);
    }
    close(c->epfd);
}

static void *dummy_thread(struct thread *t, void *arg)
{
    struct uvc_ctx *uvc = arg;
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;
    struct video_frame *frame;

    if (uvc_dummy_poll_init(c) == -1) {
        printf("uvc_dummy_poll_init failed!\n");
    }
    frame = video_frame_create(uvc->conf.format, uvc->conf.width, uvc->conf.height, MEDIA_MEM_DEEP);
    if (!frame) {
        printf("video_frame_create failed!\n");
        return NULL;
    }
    c->is_streaming = true;
    while (c->is_streaming) {
        if (uvc_dummy_enqueue(uvc, NULL, 0) != 0) {
            printf("uvc_dummy_enqueue failed\n");
            continue;
        }
        if (uvc_dummy_poll(uvc, -1) != 0) {
            printf("uvc_dummy_poll failed\n");
            continue;
        }

        if (uvc_dummy_dequeue(uvc, frame) == -1) {
            printf("uvc_dummy_dequeue failed\n");
        }
        uvc->on_video_frame(uvc, frame);
    }
    video_frame_destroy(frame);
    uvc_dummy_poll_deinit(c);
    return NULL;
}

static int uvc_dummy_start_stream(struct uvc_ctx *uvc)
{
    uint64_t notify = '1';
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;

    if (c->is_streaming) {
        printf("dummy is streaming already!\n");
        return -1;
    }

    if (write(c->ev_fd, &notify, sizeof(uint64_t)) != sizeof(uint64_t)) {
        printf("%s failed to notify ev_fd=%d, %d\n", __func__, c->ev_fd, errno);
        return -1;
    }

    if (uvc->on_video_frame) {
        c->thread = thread_create(dummy_thread, uvc);
    }

    return 0;
}

static int uvc_dummy_stop_stream(struct uvc_ctx *uvc)
{
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;
    uint64_t notify = '1';

    if (!c->is_streaming) {
        printf("dummy stream stopped already!\n");
        return -1;
    }

    if (write(c->ev_fd, &notify, sizeof(uint64_t)) != sizeof(uint64_t)) {
        printf("%s failed to notify ev_fd=%d, %d\n", __func__, c->ev_fd, errno);
        return -1;
    }

    if (uvc->on_video_frame) {
        c->is_streaming = false;
        thread_join(c->thread);
        thread_destroy(c->thread);
    }

    return 0;
}

static int uvc_dummy_query_frame(struct uvc_ctx *uvc, struct video_frame *frame)
{
    if (uvc_dummy_enqueue(uvc, NULL, 0) != 0) {
        printf("uvc_dummy_enqueue failed\n");
        return -1;
    }
    if (uvc_dummy_dequeue(uvc, frame) == -1) {
        printf("uvc_dummy_dequeue failed\n");
        return -1;
    }
    return 0;
}

static void uvc_dummy_close(struct uvc_ctx *uvc)
{
    struct dummy_ctx *c = (struct dummy_ctx *)uvc->opaque;
    uvc_dummy_stop_stream(uvc);
    close(c->fd);
    close(c->epfd);
    close(c->ev_fd);
    free(c);
}

struct uvc_ops dummy_ops = {
    .open         = uvc_dummy_open,
    .close        = uvc_dummy_close,
    .ioctl        = NULL,
    .start_stream = uvc_dummy_start_stream,
    .stop_stream  = uvc_dummy_stop_stream,
    .query_frame  = uvc_dummy_query_frame,
};
