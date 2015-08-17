/*******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libjpeg-turbo-ext.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-28 16:07
 * updated: 2015-07-28 16:07
 ******************************************************************************/
#ifndef _LIBJPEG_TURBO_EXT_H_
#define _LIBJPEG_TURBO_EXT_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/uio.h>
#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

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


typedef struct jpeg {
    struct iovec data;
    int width;
    int height;
} jpeg_t;

typedef struct yuv {
  int format;
  int sub_format;
  struct iovec luma;
  struct iovec chroma;
  int width;
  int height;
  int pitch;
  uint32_t y_addr_offset;
  uint32_t uv_addr_offset;
} yuv_t;


struct yuv *yuv_new(int format, int width, int height);
void yuv_free(struct yuv *yuv);

struct jpeg *jpeg_new(int width, int height);
void jpeg_free(struct jpeg *);
int jpeg_encode(struct jpeg *jpeg, struct yuv *yuv);

#ifdef __cplusplus
}
#endif
#endif
