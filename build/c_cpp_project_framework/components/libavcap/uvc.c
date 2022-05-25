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
#define UVC_DEBUGGING
#include <libuvc/libuvc.h>

struct uvc_ctx {
    uvc_context_t       *uvc;
    uvc_device_t        *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t    ctrl;
    struct media_frame   frame;
    struct avcap_ctx    *parent;

};

static void *_uvc_open(struct avcap_ctx *avcap, const char *dev, struct avcap_config *avconf)
{
    uvc_error_t res;
    struct videocap_config *conf = &avconf->video;
    struct uvc_ctx *c = calloc(1, sizeof(struct uvc_ctx));
    if (!c) {
        printf("malloc uvc_ctx failed!\n");
        return NULL;
    }
    res = uvc_init(&c->uvc, NULL);
    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return NULL;
    }

    /* filter devices: vendor_id, product_id, "serial_num" */
    res = uvc_find_device(c->uvc, &c->dev, 0, 0, NULL);
    if (res < 0) {
        uvc_perror(res, "uvc_find_device"); /* no devices found */
        return NULL;
    }
    res = uvc_open(c->dev, &c->devh);
    if (res < 0) {
        uvc_perror(res, "uvc_open"); /* unable to open device */
        return NULL;
    }
    //uvc_print_diag(c->devh, stderr);

    avcap->conf.video.width = avconf->video.width;
    avcap->conf.video.height = avconf->video.height;
    avcap->conf.video.fps.num = avconf->video.fps.num;
    avcap->conf.video.fps.den = avconf->video.fps.den;
    avcap->conf.video.format = avconf->video.format;

    res = uvc_get_stream_ctrl_format_size(
                    c->devh, &c->ctrl, /* result stored in ctrl */
                    UVC_FRAME_FORMAT_YUYV, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
                    640, 480, 30 /* width, height, fps */
                    );
    if (res < 0) {
        uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
    }
    uvc_print_stream_ctrl(&c->ctrl, stderr);

    c->parent = avcap;
    c->frame.type = MEDIA_TYPE_VIDEO;
    video_frame_init(&c->frame.video, conf->format, conf->width, conf->height, MEDIA_MEM_SHALLOW);
    return c;
}

static void _uvc_close(struct avcap_ctx *avcap)
{
    struct uvc_ctx *c = (struct uvc_ctx *)avcap->opaque;

    uvc_close(c->devh);
    uvc_exit(c->uvc);
    free(c);
}

static int uvc_ioctl(struct avcap_ctx *avcap, unsigned long int cmd, ...)
{
    printf("uvc_ioctl unsupport cmd!\n");
    return 0;
}

static void uvc_stream_cb(uvc_frame_t *org_frame, void *ptr)
{
    struct uvc_ctx *c = (struct uvc_ctx *)ptr;
    struct avcap_ctx *avcap = c->parent;
    struct videocap_config *conf = &avcap->conf.video;
    int i;
    uint8_t *start;
    struct media_frame *media = &c->frame;
    struct video_frame *frame = &media->video;
    media->type = MEDIA_TYPE_VIDEO;
    video_frame_init(frame, conf->format, conf->width, conf->height, MEDIA_MEM_SHALLOW);
    frame->frame_id = org_frame->sequence;
#define timeval2ns(tv) \
    (((uint64_t)tv.tv_sec * 1000000000) + ((uint64_t)tv.tv_usec * 1000))


    frame->timestamp = timeval2ns(org_frame->capture_time);
    printf("capture_time = %ld, %ld\n", org_frame->capture_time.tv_sec, org_frame->capture_time.tv_usec);
    start = org_frame->data;
    if (avcap->on_media_frame) {
        for (i = 0; i < frame->planes; ++i) {
            frame->data[i] = start + frame->plane_offsets[i];
        }
        avcap->on_media_frame(avcap, media);
    }
#if 0
    /* We'll convert the image from YUV/JPEG to BGR, so allocate space */
    bgr = uvc_allocate_frame(frame->width * frame->height * 3);
    if (!bgr) {
            printf("unable to allocate bgr frame!");
            return;
    }

    /* Do the BGR conversion */
    ret = uvc_any2bgr(frame, bgr);
    if (ret) {
            uvc_perror(ret, "uvc_any2bgr");
            uvc_free_frame(bgr);
            return;
    }

  /* Call a user function:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_user_function(ptr, bgr);
   * my_other_function(ptr, bgr->data, bgr->width, bgr->height);
   */

  /* Call a C++ method:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_obj->my_func(bgr);
   */

  /* Use opencv.highgui to display the image:
   * 
   * cvImg = cvCreateImageHeader(
   *     cvSize(bgr->width, bgr->height),
   *     IPL_DEPTH_8U,
   *     3);
   *
   * cvSetData(cvImg, bgr->data, bgr->width * 3); 
   *
   * cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
   * cvShowImage("Test", cvImg);
   * cvWaitKey(10);
   *
   * cvReleaseImageHeader(&cvImg);
   */

  uvc_free_frame(bgr);
#endif
}

static int uvc_start_stream(struct avcap_ctx *avcap)
{
    struct uvc_ctx *c = (struct uvc_ctx *)avcap->opaque;
    uvc_error_t res;

    res = uvc_start_streaming(c->devh, &c->ctrl, uvc_stream_cb, c, 0);
    if (res < 0) {
        uvc_perror(res, "start_streaming"); /* unable to start stream */
    }
    printf("%s:%d xxx!\n", __func__, __LINE__);
    return 0;
}

static int uvc_stop_stream(struct avcap_ctx *avcap)
{
    struct uvc_ctx *c = (struct uvc_ctx *)avcap->opaque;
    uvc_stop_streaming(c->devh);
    return 0;
}

struct avcap_ops uvc_ops = {
    ._open        = _uvc_open,
    ._close       = _uvc_close,
    .ioctl        = uvc_ioctl,
    .start_stream = uvc_start_stream,
    .stop_stream  = uvc_stop_stream,
#if 0
    .query_frame  = avcap_uvc_query_frame,
#endif
};
