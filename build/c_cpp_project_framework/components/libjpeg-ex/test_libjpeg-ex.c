/*******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libfoo.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-12 00:56
 * updated: 2015-07-12 00:56
 ******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libjpeg-ex.h"
#define WIDTH 	1920
#define HEIGH	1080

#if 0
void get_luma_buffer(void *buf, struct yuv *yuv)
{
#if 0
    void *in = NULL;
    void *out = NULL;
    if (yuv->pitch == yuv->width) {
      memcpy(buf, yuv->y_addr_offset, yuv->width * yuv->height);
    } else {
      in = yuv->y_addr_offset;
      out = output;
      for (i = 0; i < yuv->height; i++) {
        memcpy(out, in, yuv->width);
        in += yuv->pitch;
        out += yuv->width;
      }
    }
#endif
}

void get_chroma_buffer(void *output, struct yuv *yuv)
{
#if 0
    int width = 0;
    int height = 0;
    int pitch = 0;
    struct yuv_neon_arg yuv_neon;
    if (yuv->format == YUV_FORMAT_YUV420) {
      width = yuv->width / 2;
      height = yuv->height / 2;
      pitch = yuv->pitch / 2;
      yuv_neon.in = yuv->uv_addr_offset;
      yuv_neon.row = height;
      yuv_neon.col = yuv->width;
      yuv_neon.pitch = yuv->pitch;
      if (yuv->sub_format == YUV420_YV12) {
        // YV12 format (YYYYYYYYVVUU)
        yuv_neon.u = output + width * height;
        yuv_neon.v = output;
        chrome_convert(&yuv_neon);
      } else if (yuv->sub_format == YUV420_IYUV) {
        // IYUV (I420) format (YYYYYYYYUUVV)
        yuv_neon.u = output;
        yuv_neon.v = output + width * height;
        chrome_convert(&yuv_neon);
      } else {
        if (yuv->sub_format != YUV420_NV12) {
          INFO("Change output format back to NV12 for encode!\n");
          yuv->sub_format = YUV420_NV12;
        }
        // NV12 format (YYYYYYYYUVUV)
        input = yuv->uv_addr_offset;
        for (i = 0; i < height; ++i) {
          memcpy(output + i * width * 2, input + i * pitch * 2, width * 2);
        }
      }
    } else if (yuv->format == YUV422) {
      width = yuv->width / 2;
      height = yuv->height;
      pitch = yuv->pitch / 2;
      yuv_neon.in = yuv->uv_addr_offset;
      yuv_neon.row = height;
      yuv_neon.col = yuv->width;
      yuv_neon.pitch = yuv->pitch;
      if (yuv->sub_format == YUV422_YU16) {
        yuv_neon.u = output;
        yuv_neon.v = output + width * height;
      } else {
        if (yuv->sub_format != YUV422_YV16) {
          printf("Change output format back to YV16 for preview!\n");
          yuv->sub_format = YUV422_YV16;
        }
        yuv_neon.u = output + width * height;
        yuv_neon.v = output;
      }
      chrome_convert(&yuv_neon);
    } else {
      printf("Error: Unsupported YUV input format!\n");
    }
#endif
}

void get_yuv_buffer(struct yuv *yuv)
{
    //get_luma_buffer(luma, yuv);
    //get_chroma_buffer(chroma, yuv);

}

int get_yuv_file(struct yuv *yuv, const char *name)
{
    int fd = -1;
    if ((fd = open(name, O_RDWR, 0644)) < 0) {
      perror("Failed to open file\n");
      return -1;
    }
    if (read(fd, yuv->luma.iov_base, yuv->luma.iov_len) < 0) {
      perror("Failed to sava ME y data into file\n");
      return -1;
    }
    if (read(fd, yuv->chroma.iov_base, yuv->chroma.iov_len) < 0) {
      perror("Failed to save ME uv data into file !\n");
      return -1;
    }
    close(fd);
    return 0;
}
#endif

int save_jpeg_file(char *filename, struct je_jpeg *jpeg)
{
    int fd = -1;
    if ((fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
      perror("Failed to open file\n");
      return -1;
    }
    if (write(fd, jpeg->data.iov_base, jpeg->data.iov_len) < 0) {
      perror("Failed to sava data into file\n");
      return -1;
    }
    close(fd);
    return 0;
}

#if 0
void foo()
{
    logi("xxx\n");
    struct yuv *yuv = je_yuv_new(YUV420, WIDTH, HEIGH);
    get_yuv_file(yuv, "test_1920x1080.yuv");
    struct jpeg *jpeg = je_jpeg_new(WIDTH, HEIGH);
    //get_yuv_buffer(yuv);
    logi("xxx\n");
    je_encode_yuv_to_jpeg(yuv, jpeg, 60);
    logi("xxx\n");
    save_jpeg_file("720x480.jpeg", jpeg);
    //use jpeg
    je_yuv_free(yuv);
    je_jpeg_free(jpeg);
    logi("xxx\n");
}
#endif

void foo2()
{
    struct je_yuv *yuv = NULL;
    yuv = je_get_yuv_from_file("720x480.yuv",
                               YUV420, 720, 480, 480);
    struct je_jpeg *jpeg = je_jpeg_new(720, 480);
    struct je_codec *codec = je_codec_create();
    je_encode_yuv_to_jpeg(codec, yuv, jpeg);
    save_jpeg_file("720x480.jpeg", jpeg);
    je_yuv_free(yuv);
    je_jpeg_free(jpeg);
    je_codec_destroy(codec);

}

int main(int argc, char **argv)
{
    foo2();
    return 0;
}
