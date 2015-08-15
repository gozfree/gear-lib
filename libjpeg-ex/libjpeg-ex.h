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
#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jpeg {
    uint8_t *data;
    int len;
} jpeg_t;

typedef struct yuv {
  uint8_t *luma_chroma;
  int luma_len;
  int chroma_len;
  int width;
  int height;
  int pitch;
} yuv_t;


struct yuv *yuv_new();
void yuv_free(struct yuv *);

struct jpeg *jpeg_new(int width, int height);
void jpeg_free(struct jpeg *);
int jpeg_encode(struct jpeg *jpeg, struct yuv *yuv);

#ifdef __cplusplus
}
#endif
#endif
