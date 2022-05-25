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
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#if !defined (OS_RTOS)
#include <sys/epoll.h>
#include <sys/eventfd.h>
#endif
#include <sys/stat.h>

#if !defined (OS_RTOS)
struct dummy_ctx {
    int fd;
    int ev_fd;
    uint64_t seek_offset;
    uint64_t file_size_total;
    uint64_t first_ts;
    uint64_t frame_id;
    struct avcap_ctx *parent;
    struct thread *thread;
    bool is_streaming;
    int epfd;
    struct epoll_event events;
};

#define timespec2ns(ts) \
    (((uint64_t)ts.tv_sec * 1000000000) + ((uint64_t)ts.tv_nsec))

static ssize_t get_file_size(const char *path)
{
    struct stat st;
    off_t size = 0;
    if (!path) {
        return -1;
    }
    if (stat(path, &st) < 0) {
        printf("%s stat error: %s\n", path, strerror(errno));
    } else {
        size = st.st_size;
    }
    return size;
}

static void *dummy_open(struct avcap_ctx *avcap, const char *dev, struct avcap_config *conf)
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
    c->file_size_total = get_file_size(dev);

    memcpy(&avcap->conf, conf, sizeof(struct avcap_config));
    c->parent = avcap;

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

static int dummy_enqueue(struct avcap_ctx *avcap, void *buf, size_t len)
{
    uint64_t notify = '1';
    uint64_t delay_ms = 0;
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;

    delay_ms = (1000)/(avcap->conf.video.fps.num/(avcap->conf.video.fps.den*1.0));
    msleep(delay_ms);
    if (write(c->ev_fd, &notify, sizeof(uint64_t)) != sizeof(uint64_t)) {
        printf("%s failed to notify ev_fd=%d, %d\n", __func__, c->ev_fd, errno);
        return -1;
    }
    return 0;
}

static int dummy_dequeue(struct avcap_ctx *avcap, struct video_frame *frame)
{
    int i;
    ssize_t ret;
    size_t len;
    uint64_t notify;
    struct timespec ts;
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;

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
        if (c->seek_offset > c->file_size_total) {
            printf("read file exceed total file size\n");
            return -1;
        }
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

static int dummy_poll_init(struct dummy_ctx *c)
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

static int dummy_poll(struct avcap_ctx *avcap, int timeout)
{
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;
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

static void dummy_poll_deinit(struct dummy_ctx *c)
{
    if (-1 == epoll_ctl(c->epfd, EPOLL_CTL_DEL, c->ev_fd, NULL)) {
        printf("epoll_ctl EPOLL_CTL_DEL failed %d!\n", errno);
    }
    close(c->epfd);
}

static void *dummy_thread(struct thread *t, void *arg)
{
    struct avcap_ctx *avcap = arg;
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;
    struct media_frame media;
    struct video_frame *frame = &media.video;
    media.type = MEDIA_TYPE_VIDEO;

    if (dummy_poll_init(c) == -1) {
        printf("avcap_dummy_poll_init failed!\n");
    }
    video_frame_init(frame, avcap->conf.video.format, avcap->conf.video.width, avcap->conf.video.height, MEDIA_MEM_DEEP);
    c->is_streaming = true;
    while (c->is_streaming) {
        if (dummy_enqueue(avcap, NULL, 0) != 0) {
            printf("dummy_enqueue failed\n");
            continue;
        }
        if (dummy_poll(avcap, -1) != 0) {
            printf("avcap_dummy_poll failed\n");
            continue;
        }

        if (dummy_dequeue(avcap, frame) == -1) {
            printf("dummy_dequeue failed\n");
            break;
        }
        avcap->on_media_frame(avcap, &media);
    }
    video_frame_deinit(frame);
    dummy_poll_deinit(c);
    return NULL;
}

static int dummy_start_stream(struct avcap_ctx *avcap)
{
    uint64_t notify = '1';
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;

    if (c->is_streaming) {
        printf("dummy is streaming already!\n");
        return -1;
    }

    if (write(c->ev_fd, &notify, sizeof(uint64_t)) != sizeof(uint64_t)) {
        printf("%s failed to notify ev_fd=%d, %d\n", __func__, c->ev_fd, errno);
        return -1;
    }

    if (avcap->on_media_frame) {
        c->thread = thread_create(dummy_thread, avcap);
    }

    return 0;
}

static int dummy_stop_stream(struct avcap_ctx *avcap)
{
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;
    uint64_t notify = '1';

    if (!c->is_streaming) {
        printf("dummy stream stopped already!\n");
        return -1;
    }

    if (write(c->ev_fd, &notify, sizeof(uint64_t)) != sizeof(uint64_t)) {
        printf("%s failed to notify ev_fd=%d, %d\n", __func__, c->ev_fd, errno);
        return -1;
    }

    if (avcap->on_media_frame) {
        c->is_streaming = false;
        thread_join(c->thread);
        thread_destroy(c->thread);
    }

    return 0;
}

static int dummy_query_frame(struct avcap_ctx *avcap, struct media_frame *frame)
{
    if (dummy_enqueue(avcap, NULL, 0) != 0) {
        printf("dummy_enqueue failed\n");
        return -1;
    }
    if (dummy_dequeue(avcap, &frame->video) == -1) {
        printf("dummy_dequeue failed\n");
        return -1;
    }
    return 0;
}

static int dummy_ioctl(struct avcap_ctx *avcap, unsigned long int cmd, ...)
{
    printf("dummy_ioctl unsupport cmd!\n");
    return -1;
}

static void dummy_close(struct avcap_ctx *avcap)
{
    struct dummy_ctx *c = (struct dummy_ctx *)avcap->opaque;
    dummy_stop_stream(avcap);
    close(c->fd);
    close(c->epfd);
    close(c->ev_fd);
    free(c);
}
#endif

struct avcap_ops dummy_ops = {
#if !defined (OS_RTOS)
    ._open        = dummy_open,
    ._close       = dummy_close,
    .ioctl        = dummy_ioctl,
    .start_stream = dummy_start_stream,
    .stop_stream  = dummy_stop_stream,
    .query_frame  = dummy_query_frame,
#endif
};
