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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jpeglib.h>
#include <libmacro.h>
#include <liblog.h>
#include "libjpeg-ex.h"

#define COLOR_COMPONENTS    (3)
#define OUTPUT_BUF_SIZE     (4096)
#define IYUV                (0)
#define NV12                (1)
#define DEFAULT_JPEG_QUALITY	(60)

typedef struct image {
    struct iovec data;
    int format;
    int width;    /* Number of columns in image */
    int height;   /* Number of rows in image */
} image_t ;

typedef struct jpeg_args {
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

//extern "C" void chrome_convert(struct yuv_neon_arg *);
#if 0
static void chrome_convert(struct yuv_neon_arg *yuv)
{
    printf("chrome_convert\n");
}
#endif

#if 0
static struct image *image_new(struct yuv *yuv)
{
    uint8_t *image_chrome = NULL;
    int u_offset = 0;
    int size_u = 0,size_v = 0;
    int width = yuv->width;
    int height = yuv->height;
    int pitch = yuv->pitch;
    yuv_neon_arg yuv_arg;
    if (width <= 0 || height <= 0 || pitch <= 0) {
        printf("invalid yuv paraments, width=%d, height=%d, pitch=%d\n", width, height, pitch);
        return NULL;
    }
    struct image *img = CALLOC(1, struct image);
    if (!img) {
        printf("malloc img failed!\n");
        return NULL;
    }

    img->width = width;
    img->height = height;

    u_offset = width * height;
    size_u = width * height / 4;
    size_v = size_u;

    img->data.iov_len = yuv->luma.iov_len + yuv->chroma.iov_len;
    img->data.iov_base = calloc(1, img->data.iov_len);
    if (!img->data.iov_base) {
        printf("malloc image data failed\n");
        return NULL;
    }
    memcpy(img->data.iov_base, yuv->luma.iov_base, yuv->luma.iov_len);
    memcpy(img->data.iov_base + yuv->luma.iov_len, yuv->chroma.iov_base, yuv->chroma.iov_len);

    if (img->format == NV12) {
        image_chrome = (uint8_t *)malloc(size_u + size_v);
        if (image_chrome == NULL) {
            printf("malloc image_u or image_v failed\n");
            return NULL;
        }
        yuv_arg.in = img->data.iov_base + u_offset;
        yuv_arg.u = image_chrome;
        yuv_arg.v = image_chrome + size_u;
        yuv_arg.row = height / 2;
        yuv_arg.col = width;
        yuv_arg.pitch = width;
        chrome_convert(&yuv_arg);
        memcpy((img->data.iov_base + u_offset), yuv_arg.u, size_u + size_v);
    } else {
        loge("img->format = %d\n", img->format);
    }
    if (image_chrome != NULL) {
        free(image_chrome);
        image_chrome = NULL;
    }
    return img;
}

static void image_free(struct image *img)
{
    if (!img) {
        return;
    }
    if (!img->data.iov_base) {
        return;
    }
    free(img->data.iov_base);
    free(img);
}
#endif

static void init_destination(j_compress_ptr cinfo)
{
    struct jpeg_args *dest = (struct jpeg_args*) cinfo->dest;
    dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));
    *(dest->written) = 0;
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
    struct jpeg_args *dest = (struct jpeg_args *) cinfo->dest;
    memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
    dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
    *(dest->written) += OUTPUT_BUF_SIZE;
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
    return TRUE;
}

static void term_destination(j_compress_ptr cinfo)
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

#if 0
static int do_jpeg_encode(struct jpeg *jpeg, struct image *img)
{
    uint8_t *output = jpeg->data.iov_base;
    int quality = DEFAULT_JPEG_QUALITY;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    int i = 0;
    int j = 0;
    int written = 0;

    uint32_t size = img->width * img->height;
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
    dest_buffer(&cinfo, output, img->width * img->height, &written);

    cinfo.image_width = img->width;	/* image width and height, in pixels */
    cinfo.image_height = img->height;
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

    for (j = 0; j < img->height; j += 16) {
        for (i = 0; i < 16; i++) {
            row = img->width * (i + j);
            y[i] = img->data.iov_base + row;
            if (i % 2 == 0) {
                cb[i/2] = img->data.iov_base + size + row/4;
                cr[i/2] = img->data.iov_base + size + quarter_size + row/4;
            }
        }
        jpeg_write_raw_data(&cinfo, planes, 16);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    jpeg->data.iov_len = written;
    return 0;
}
#endif

#if 0
int je_encode_yuv_to_jpeg(struct yuv *yuv, struct jpeg *jpeg, int quality)
{
    int res = 0;
    struct image *img = image_new(yuv);
    if (!img) {
        res = -1;
        printf("image_new failed\n");
        goto end;
    }
    if (0 != do_jpeg_encode(jpeg, img)) {
        res = -1;
        printf("image_new failed\n");
        goto end;
    }
end:
    image_free(img);
    return res;
}
#endif

#if 0
struct yuv *je_yuv_new(int format, int width, int height)
{
    struct yuv *yuv = CALLOC(1, struct yuv);
    if (!yuv) {
        loge("malloc yuv failed\n");
        goto err;
    }
    yuv->format = format;
    yuv->width = width;
    yuv->height = height;
    yuv->pitch = width;
    yuv->luma.iov_len = width * height;
    if (format == YUV420) {
        yuv->chroma.iov_len = yuv->luma.iov_len / 2;
    } else if (format == YUV422) {
        yuv->chroma.iov_len = yuv->luma.iov_len;
    } else {
        printf("invalid yuv data format!\n");
        goto err;
    }
    yuv->luma.iov_base = calloc(1, yuv->luma.iov_len);
    yuv->chroma.iov_base = calloc(1, yuv->chroma.iov_len);
    if (!yuv->luma.iov_base || !yuv->chroma.iov_base) {
        printf("malloc luma chroma failed\n");
        goto err;
    }
    return yuv;

err:
    if (yuv) {
        free(yuv);
    }
    return NULL;
}
#endif

void je_yuv_free(struct je_yuv *yuv)
{
    if (!yuv) {
        return;
    }
    if (yuv->is_continuous) {
        free(yuv->y_data.iov_base);
        free(yuv->uv_data.iov_base);
    } else {
        free(yuv->data.iov_base);
    }
    free(yuv);
}

struct je_jpeg *je_jpeg_new(int width, int height)
{
    struct je_jpeg *jpeg = CALLOC(1, struct je_jpeg);
    if (!jpeg) {
        printf("malloc jpeg failed!\n");
        goto err;
    }
    jpeg->data.iov_len = width * height * COLOR_COMPONENTS;
    jpeg->data.iov_base = calloc(1, jpeg->data.iov_len);
    if (!jpeg->data.iov_base) {
        printf("malloc jpeg buffer failed!\n");
        goto err;
    }
    return jpeg;
err:
    if (jpeg) {
        free(jpeg);
    }
    return NULL;
}

void je_jpeg_free(struct je_jpeg *jpeg)
{
    if (!jpeg) {
        return;
    }
    free(jpeg->data.iov_base);
    free(jpeg);
}

struct je_codec *je_codec_create()
{
    struct je_codec *codec = CALLOC(1, struct je_codec);
    if (!codec) {
        loge("malloc jpeg codec failed!\n");
        return NULL;
    }
    //init encoder
    codec->encoder.err = jpeg_std_error(&codec->errmgr);
    jpeg_create_compress(&codec->encoder);
    jpeg_set_quality(&codec->encoder, DEFAULT_JPEG_QUALITY, 1);

    //init decoder
    //codec->decoder.err = jpeg_std_error(&codec->errmgr);
    //jpeg_create_decompress(&codec->decoder);

    return codec;
}

void je_codec_destroy(struct je_codec *codec)
{
    if (!codec) {
        loge("jpeg codec has already been destroyed!\n");
        return;
    }
    jpeg_destroy_compress(&codec->encoder);
    jpeg_destroy_decompress(&codec->decoder);
    free(codec);
}

struct je_yuv *je_get_yuv_from_file(const char *name,
                                    int format,
                                    int width,
                                    int height,
                                    int pitch)
{
    struct je_yuv *yuv = NULL;
    struct stat st;
    int fd = -1;
    if ((fd = open(name, O_RDWR, 0644)) < 0) {
        loge("failed to open %s\n", name);
        return NULL;
    }
    memset(&st, 0, sizeof(st));
    if (-1 == fstat(fd, &st) || st.st_size <= 0) {
        loge("failed to get %s information!\n", name);
        goto err;
    }
    yuv = CALLOC(1, struct je_yuv);
    if (!yuv) {
        loge("malloc yuv failed!\n");
        goto err;
    }
    yuv->format = format;
    yuv->width = width;
    yuv->height = height;
    yuv->pitch = pitch;
    yuv->is_continuous = 1;

    loge("st.st_size=%d, width*height=%d\n", st.st_size, width*height);
    yuv->data.iov_len = st.st_size;
    //yuv->data.iov_len = width *height;
    yuv->data.iov_base = calloc(1, yuv->data.iov_len);

    if (width*height != yuv->data.iov_len) {
        logw("yuv file size does not equal width*height!\n");
    }
    if (read(fd, yuv->data.iov_base, yuv->data.iov_len) < 0) {
        loge("failed to read yuv data\n");
        goto err;
    }
err:
    close(fd);
    return yuv;
}

int je_encode_yuv_to_jpeg(struct je_codec *codec, struct je_yuv *yuv, struct je_jpeg *jpeg)
{
    if (!codec || !yuv || !jpeg) {
        loge("invalid paraments!\n");
        return -1;
    }
    uint8_t *output = jpeg->data.iov_base;
    int quality = DEFAULT_JPEG_QUALITY;
    struct jpeg_compress_struct *encoder = &codec->encoder;

    int i = 0;
    int j = 0;
    int written = 0;

    uint32_t size = yuv->width * yuv->height;
    uint32_t quarter_size = size / 4;
    uint32_t row = 0;

    JSAMPROW y[16];
    JSAMPROW cb[16];
    JSAMPROW cr[16];
    JSAMPARRAY planes[3];

    planes[0] = y;
    planes[1] = cb;
    planes[2] = cr;

    dest_buffer(encoder, output, yuv->width * yuv->height, &written);

    encoder->image_width = yuv->width;	/* image width and height, in pixels */
    encoder->image_height = yuv->height;
    encoder->input_components = COLOR_COMPONENTS;	/* # of color components per pixel */
    encoder->in_color_space = JCS_YCbCr;       /* colorspace of input image */

    jpeg_set_defaults(encoder);
    encoder->raw_data_in = TRUE;	// supply downsampled data
    encoder->dct_method = JDCT_IFAST;

    encoder->comp_info[0].h_samp_factor = 2;
    encoder->comp_info[0].v_samp_factor = 2;
    encoder->comp_info[1].h_samp_factor = 1;
    encoder->comp_info[1].v_samp_factor = 1;
    encoder->comp_info[2].h_samp_factor = 1;
    encoder->comp_info[2].v_samp_factor = 1;

    jpeg_set_quality(encoder, quality, TRUE /* limit to baseline-JPEG values */);
    loge("xxx\n");
    jpeg_start_compress(encoder, TRUE);
    loge("xxx\n");

    for (j = 0; j < yuv->height; j += 16) {
        for (i = 0; i < 16; i++) {
            row = yuv->width * (i + j);
            y[i] = yuv->data.iov_base + row;
            if (i % 2 == 0) {
                cb[i/2] = yuv->data.iov_base + size + row/4;
                cr[i/2] = yuv->data.iov_base + size + quarter_size + row/4;
            }
        }
        jpeg_write_raw_data(encoder, planes, 16);
    }
    loge("xxx\n");
    jpeg_finish_compress(encoder);
    loge("xxx\n");
    jpeg->data.iov_len = written;
    return 0;
}
