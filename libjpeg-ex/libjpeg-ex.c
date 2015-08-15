/*******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libjpeg-turbo-ext.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-12 00:56
 * updated: 2015-07-12 00:56
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <jpeglib.h>
#include <libgzf.h>
#include "libjpeg-ex.h"

#define COLOR_COMPONENTS    (3)
#define OUTPUT_BUF_SIZE     (4096)
#define IYUV                (0)
#define NV12                (1)

typedef struct image {
  uint8_t *buffer;
  int format;
  int width;    /* Number of columns in image */
  int height;   /* Number of rows in image */
} image_t ;

typedef struct jpeg_args{
  struct jpeg_destination_mgr pub; /* public fields */
  JOCTET * buffer;    /* start of buffer */
  unsigned char *outbuffer;
  int outbuffer_size;
  unsigned char *outbuffer_cursor;
  int *written;
} jpeg_args_t;

typedef struct yuv_neon_arg {
  uint8_t *in;
  uint8_t *u;
  uint8_t *v;
  int row;
  int col;
  int pitch;
} yuv_neon_arg;

typedef enum {
    YUV420_IYUV = 0,	// Pattern: YYYYYYYYUUVV
    YUV420_YV12 = 1,	// Pattern: YYYYYYYYVVUU
    YUV420_NV12 = 2,	// Pattern: YYYYYYYYUVUV
    YUV422_YU16 = 3,	// Pattern: YYYYYYYYUUUUVVVV
    YUV422_YV16 = 4,	// Pattern: YYYYYYYYVVVVUUUU
    YUV_FORMAT_TOTAL_NUM,
} YUV_FORMAT;

//extern "C" void chrome_convert(struct yuv_neon_arg *);
static void chrome_convert(struct yuv_neon_arg *yuv)
{

}

static struct image *image_new(uint8_t *input, int width, int height, int pitch)
{
    uint32_t size_input = 0;
    uint8_t *image_chrome = NULL;
    int u_offset = 0;
    int size_u = 0,size_v = 0;
    yuv_neon_arg yuv_arg;
    if ((width <= 0) ||(height <= 0) ||(pitch <= 0)) {
        printf("invalid yuv paraments, width=%d, height=%d, pitch=%d\n", width, height, pitch);
        return NULL;
    }
    struct image *image_buf = CALLOC(1, struct image);
    if (!image_buf) {
        printf("malloc image_buf failed!\n");
        return NULL;
    }

    size_input = width * height * 3/2;
    image_buf->width = width;
    image_buf->height = height;

    u_offset = width * height;
    size_u = width * height / 4;
    size_v = size_u;

    do {
        image_buf->buffer = (uint8_t *) malloc(size_input);
        if (!image_buf->buffer) {
            printf("Not enough memory for image buffer.\n");
            break;
        }
        memcpy(image_buf->buffer, input, size_input);

        if (image_buf->format == NV12) {
            image_chrome = (uint8_t *)malloc(size_u + size_v);
            if (image_chrome == NULL) {
                printf("Not enough memory for image_u or image_v.\n");
                return NULL;
            }
            yuv_arg.in = image_buf->buffer + u_offset;
            yuv_arg.u = image_chrome;
            yuv_arg.v = image_chrome + size_u;
            yuv_arg.row = height / 2;
            yuv_arg.col = width;
            yuv_arg.pitch = width;
            chrome_convert(&yuv_arg);
            memcpy((image_buf->buffer + u_offset), yuv_arg.u, size_u + size_v);
        }
    } while(0);
    if (image_chrome != NULL) {
        free(image_chrome);
        image_chrome = NULL;
    }
    return image_buf;
}

static void image_free(struct image *image_buf)
{
    if (!image_buf) {
        return;
    }
    if (!image_buf->buffer) {
        return;
    }
    free(image_buf->buffer);
    free(image_buf);
}

static void init_destination(j_compress_ptr cinfo)
{
    struct jpeg_args *dest = (struct jpeg_args*) cinfo->dest;
    dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));
    *(dest->written) = 0;
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

boolean empty_output_buffer(j_compress_ptr cinfo)
{
    struct jpeg_args *dest = (struct jpeg_args *) cinfo->dest;

    memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
    dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
    *(dest->written) += OUTPUT_BUF_SIZE;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

    return TRUE;
}

void term_destination(j_compress_ptr cinfo)
{
    struct jpeg_args * dest = (struct jpeg_args *) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

    memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
    dest->outbuffer_cursor += datacount;
    *(dest->written) += datacount;
}

static void dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written)
{
    struct jpeg_args * dest;
    if (cinfo->dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_args));
    }

    dest = (struct jpeg_args*) cinfo->dest;
    dest->pub.init_destination = init_destination;
    dest->pub.empty_output_buffer = empty_output_buffer;
    dest->pub.term_destination = term_destination;
    dest->outbuffer = buffer;
    dest->outbuffer_size = size;
    dest->outbuffer_cursor = buffer;
    dest->written = written;
}

static int do_jpeg_encode(struct jpeg *jpeg, struct image *input)
{
#define DEFAULT_JPEG_QUALITY	(60)
    uint8_t *output = jpeg->data;
    int quality = DEFAULT_JPEG_QUALITY;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    int i = 0;
    int j = 0;
    int written = 0;

    uint32_t size = input->width * input->height;
    uint32_t quarter_size = size / 4;
    uint32_t row = 0;

    JSAMPROW y[16];
    JSAMPROW cb[16];
    JSAMPROW cr[16];
    JSAMPARRAY planes[3];

    planes[0] = y;
    planes[1] = cb;
    planes[2] = cr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    dest_buffer(&cinfo, output, input->width * input->height, &written);

    cinfo.image_width = input->width;	/* image width and height, in pixels */
    cinfo.image_height = input->height;
    cinfo.input_components = COLOR_COMPONENTS;	/* # of color components per pixel */
    cinfo.in_color_space = JCS_YCbCr;       /* colorspace of input image */

    jpeg_set_defaults(&cinfo);
    cinfo.raw_data_in = TRUE;	// supply downsampled data
    cinfo.dct_method = JDCT_IFAST;

    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;
    cinfo.comp_info[2].v_samp_factor = 1;

    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
    jpeg_start_compress(&cinfo, TRUE);

    for (j = 0; j < input->height; j += 16) {
        for (i = 0; i < 16; i++) {
            row = input->width * (i + j);
            y[i] = input->buffer + row;
            if (i % 2 == 0) {
                cb[i/2] = input->buffer + size + row/4;
                cr[i/2] = input->buffer + size + quarter_size + row/4;
            }
        }
        jpeg_write_raw_data(&cinfo, planes, 16);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    jpeg->len = written;
    return written;
}

int jpeg_encode(struct jpeg *jpeg_buf, struct yuv *yuv_buf)
{
    int res = -1;
    do {
        struct image *image_buf = image_new(yuv_buf->luma_chroma, yuv_buf->width, yuv_buf->height, yuv_buf->pitch);
        if (!image_buf) {
            break;
        }

        res = do_jpeg_encode(jpeg_buf, image_buf);
        if (res == 0) {
            break;
        }
        image_free(image_buf);
    } while (0);

    return res;
}

struct yuv *yuv_new()
{
    struct yuv *yuv = CALLOC(1, struct yuv);
    if (!yuv) {
        printf("malloc yuv failed\n");
        return NULL;
    }
    return yuv;
}

void yuv_free(struct yuv *yuv)
{
    if (!yuv) {
        return;
    }
    free(yuv->luma_chroma);
    free(yuv);
}

struct jpeg *jpeg_new(int width, int height)
{
    struct jpeg *jpeg = CALLOC(1, struct jpeg);
    uint8_t *buf = (uint8_t *)calloc(1, width * height * COLOR_COMPONENTS);
    if (!buf) {
        printf("Not enough memory for jpeg buffer.\n");
        return NULL;
    }
    jpeg->data = buf;
    jpeg->len = 0;
    return jpeg;
}

void jpeg_free(struct jpeg *jpeg)
{
    if (!jpeg) {
        return;
    }
    free(jpeg->data);
    free(jpeg);
}
