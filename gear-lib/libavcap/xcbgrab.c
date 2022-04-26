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

#include <errno.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>

#include <sys/ipc.h>
#include <sys/shm.h>

struct xcbgrab_ctx {
    xcb_connection_t *xcb;
    xcb_screen_t     *xcb_screen;
    xcb_shm_seg_t segment;
    uint8_t *buffer;
    int x, y;
    int width, height;
    int stride;
    int pix_fmt;
    int bits_per_pixel;
    int frame_size;
    int shmid;
};

static bool _xcb_check_extensions(xcb_connection_t *xcb)
{
    bool ok = true;

    if (!xcb_get_extension_data(xcb, &xcb_shm_id)->present) {
        printf("Missing SHM extension !\n");
        ok = false;
    }

    if (!xcb_get_extension_data(xcb, &xcb_xinerama_id)->present)
        printf("Missing Xinerama extension !\n");

    if (!xcb_get_extension_data(xcb, &xcb_randr_id)->present)
        printf("Missing Randr extension !\n");

    return ok;
}

static xcb_screen_t *get_screen(const xcb_setup_t *setup, int screen_num)
{
    xcb_screen_t *screen = NULL;
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);

    for (; it.rem > 0; xcb_screen_next(&it)) {
        if (!screen_num) {
            screen = it.data;
            break;
        }
        screen_num--;
    }
    return screen;
}

static int xcb_get_pixfmt(struct xcbgrab_ctx *c)
{
    int i;
    const xcb_setup_t *setup = xcb_get_setup(c->xcb);
    const xcb_format_t *fmt  = xcb_setup_pixmap_formats(setup);
    int length               = xcb_setup_pixmap_formats_length(setup);

    xcb_get_geometry_cookie_t gc = xcb_get_geometry(c->xcb, c->xcb_screen->root);
    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(c->xcb, gc, NULL);

    c->width = c->xcb_screen->width_in_pixels;
    c->height = c->xcb_screen->height_in_pixels;

    if (c->x + c->width > geo->width || c->y + c->height > geo->height) {
        printf("Capture area %dx%d at position %d.%d "
               "outside the screen size %dx%d\n",
               c->width, c->height,
               c->x, c->y,
               geo->width, geo->height);
        return -1;
    }

    c->pix_fmt = 0;

    for (i = 0; i < length; i++, fmt++) {
        if (fmt->depth != geo->depth)
            continue;

        switch (geo->depth) {
        case 32:
            if (fmt->bits_per_pixel == 32)
                c->pix_fmt = PIXEL_FORMAT_0RGB;
            break;
        case 24:
            if (fmt->bits_per_pixel == 32)
                c->pix_fmt = PIXEL_FORMAT_0RGB;
            else if (fmt->bits_per_pixel == 24)
                c->pix_fmt = PIXEL_FORMAT_RGB24;
            break;
        case 16:
            if (fmt->bits_per_pixel == 16)
                    c->pix_fmt = PIXEL_FORMAT_RGB565BE;
            break;
        case 15:
            if (fmt->bits_per_pixel == 16)
                    c->pix_fmt = PIXEL_FORMAT_RGB555BE;
            break;
        case 8:
            if (fmt->bits_per_pixel == 8)
                    c->pix_fmt = PIXEL_FORMAT_RGB8;
            break;
        default:
            printf("unsupport depth=%d, bits_per_pixel=%d", fmt->depth, fmt->bits_per_pixel);
            break;
        }

        if (c->pix_fmt) {
            c->bits_per_pixel = fmt->bits_per_pixel;
            c->frame_size = c->width * c->height * fmt->bits_per_pixel / 8;
            c->stride = c->width * (c->bits_per_pixel >> 3);
            printf("width=%d, height=%d, stride=%d\n", c->width, c->height, c->stride);
            printf("capture area %dx%d at position %d.%d, screen size %dx%d\n"
                   "depth = %d, %d, bits_per_pixel = %d\n",
                   c->width, c->height, c->x, c->y, geo->width, geo->height,
                   geo->depth, fmt->depth, fmt->bits_per_pixel);
            return 0;
        }
    }

    return -1;
}

static void create_stream(struct xcbgrab_ctx *c)
{
    xcb_get_pixfmt(c);
}


static int alloc_buffer(struct xcbgrab_ctx *c)
{
    int size = c->stride * c->height;

    c->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (c->shmid == -1) {
        printf("Cannot get %d bytes of shared memory: %s.\n", size, strerror(errno));
        return -1;
    }
    xcb_shm_attach(c->xcb, c->segment, c->shmid, 0);
    c->buffer = shmat(c->shmid, NULL, 0);
    //shmctl(c->shmid, IPC_RMID, 0);
    return 0;
}

#if 0
static void free_buffer(struct xcbgrab_ctx *c)
{
    xcb_shm_detach(c->xcb, c->segment);

    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);

    xcb_disconnect(connection);
}
#endif

static int xcbgrab_frame(struct xcbgrab_ctx *c)
{
    xcb_get_image_cookie_t iq;
    xcb_get_image_reply_t *img;
    xcb_drawable_t drawable = c->xcb_screen->root;
    xcb_generic_error_t *e = NULL;
    uint8_t *data;
    int length;

    iq  = xcb_get_image(c->xcb, XCB_IMAGE_FORMAT_Z_PIXMAP, drawable,
                        c->x, c->y, c->width, c->height, ~0);

    img = xcb_get_image_reply(c->xcb, iq, &e);

    if (e) {
        printf("Cannot get the image data "
               "event_error: response_type:%u error_code:%u "
               "sequence:%u resource_id:%u minor_code:%u major_code:%u.\n",
               e->response_type, e->error_code,
               e->sequence, e->resource_id, e->minor_code, e->major_code);
        return -1;
    } else {
        printf("xcb_get_image_reply ok!\n");
    }

    if (!img)
        return -1;

    data   = xcb_get_image_data(img);
    data = data;
    length = xcb_get_image_data_length(img);
    length = length;

    free(img);

    return 0;
}
static int xcbgrab_frame_shm(struct xcbgrab_ctx *c)
{
    xcb_shm_get_image_cookie_t iq;
    xcb_shm_get_image_reply_t *img;
    xcb_drawable_t drawable = c->xcb_screen->root;
    xcb_generic_error_t *e = NULL;
    int ret;

    ret = alloc_buffer(c);
    if (ret < 0)
        return ret;

    iq = xcb_shm_get_image(c->xcb, drawable,
                           c->x, c->y, c->width, c->height, ~0,
                           XCB_IMAGE_FORMAT_Z_PIXMAP, c->segment, 0);
    img = xcb_shm_get_image_reply(c->xcb, iq, &e);

    //xcb_flush(c->xcb);

    if (e) {
        printf("Cannot get the image data "
               "event_error: response_type:%u error_code:%u "
               "sequence:%u resource_id:%u minor_code:%u major_code:%u.\n",
               e->response_type, e->error_code,
               e->sequence, e->resource_id, e->minor_code, e->major_code);

        return -1;
    }

    free(img);

    //pkt->data = c->buffer;
    //pkt->size = c->frame_size;

    return 0;
}
static void *xcbgrab_open(struct avcap_ctx *avcap, const char *dev, struct avcap_config *avconf)
{
    const xcb_setup_t *setup;
    int screen_num;
    struct xcbgrab_ctx *c = calloc(1, sizeof(struct xcbgrab_ctx));
    if (!c) {
        printf("malloc xcbgrab failed!\n");
        return NULL;
    }
    c->xcb = xcb_connect(dev, &screen_num);
    if (!c->xcb || xcb_connection_has_error(c->xcb)) {
        printf("xcb_connect X display failed!\n");
        return NULL;
    }
    printf("xcb_connect X display %d success!\n", screen_num);

    _xcb_check_extensions(c->xcb);

    setup = xcb_get_setup(c->xcb);

    c->xcb_screen = get_screen(setup, screen_num);
    if (!c->xcb_screen) {
        printf("The screen %d does not exist.\n", screen_num);
        return NULL;
    }

    create_stream(c);
    //alloc_buffer(c);
    if (0)
            xcbgrab_frame_shm(c);
    xcbgrab_frame(c);
    printf("The screen %d does not exist.\n", screen_num);

    return c;
}

static void xcbgrab_close(struct avcap_ctx *avcap)
{

}

static int xcbgrab_ioctl(struct avcap_ctx *avcap, unsigned long int cmd, ...)
{
    printf("xcbgrab_ioctl unsupport cmd!\n");
    return 0;
}

static int xcbgrab_start_stream(struct avcap_ctx *avcap)
{
    return 0;
}

static int xcbgrab_stop_stream(struct avcap_ctx *avcap)
{
    return 0;
}

struct avcap_ops xcbgrab_ops = {
    ._open        = xcbgrab_open,
    ._close       = xcbgrab_close,
    .ioctl        = xcbgrab_ioctl,
    .start_stream = xcbgrab_start_stream,
    .stop_stream  = xcbgrab_stop_stream,
#if 0
    .query_frame  = avcap_xcbgrab_query_frame,
#endif
};
