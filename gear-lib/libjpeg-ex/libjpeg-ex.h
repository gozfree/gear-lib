/*******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libjpeg-turbo-ext.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-28 16:07
 * updated: 2015-07-28 16:07
 ******************************************************************************/
#ifndef LIBJPEG_TURBO_EXT_H
#define LIBJPEG_TURBO_EXT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/uio.h>
#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In order to distinguish the jpeg_xxx API of libjpeg-turbo,
 * we use je_xxx as the namespace of API, means jpeg extension.
 */


typedef enum yuv_format {
    YUV422 = 0,
    YUV420 = 1,
} yuv_format_t;

typedef enum yuv420_format {
    YUV420_IYUV = 0,	// Pattern: YYYYYYYYUUVV
    YUV420_YV12 = 1,	// Pattern: YYYYYYYYVVUU
    YUV420_NV12 = 2,	// Pattern: YYYYYYYYUVUV
    YUV422_YU16 = 3,	// Pattern: YYYYYYYYUUUUVVVV
    YUV422_YV16 = 4,	// Pattern: YYYYYYYYVVVVUUUU
} yuv420_format_t;


struct je_jpeg {
    struct iovec data;
    int width;
    int height;
};

struct je_yuv {
    int format;
    int sub_format;
    int is_continuous;

    //if y and uv buffer seperated
    struct iovec y_data;
    struct iovec uv_data;

    //if y and uv buffer continues
    struct iovec data;

    int width;
    int height;
    int pitch;
};

struct je_codec {
    struct jpeg_compress_struct   encoder;
    struct jpeg_decompress_struct decoder;
    struct jpeg_error_mgr         errmgr;
};


struct je_codec *je_codec_create();
void je_codec_destroy(struct je_codec *codec);

struct je_yuv *je_yuv_new(int format, int width, int height);
void je_yuv_free(struct je_yuv *yuv);

struct je_jpeg *je_jpeg_new(int width, int height);
void je_jpeg_free(struct je_jpeg *jpeg);

struct je_yuv *je_get_yuv_from_file(const char *name,
                                    int format,
                                    int width,
                                    int height,
                                    int pitch);

int je_get_yuv_from_mem();

int je_encode_yuv_to_jpeg(struct je_codec *codec, struct je_yuv *in, struct je_jpeg *out);
int je_decode_jpeg_to_yuv(struct je_codec *codec, struct je_jpeg *in, struct je_yuv *out);



#ifdef __cplusplus
}
#endif
#endif
