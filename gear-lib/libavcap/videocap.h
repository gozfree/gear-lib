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
#ifndef VIDEOCAP_H
#define VIDEOCAP_H

#include <libposix.h>
#include <libmedia-io.h>

#ifdef __cplusplus
extern "C" {
#endif

struct videocap_config {
    enum pixel_format format;
    uint32_t          width;
    uint32_t          height;
    rational_t        fps;
	const char       *dev;
};

struct videocap_image_quality {
    int brightness;
    int contrast;
    int saturation;
    int hue;
    int awb;
    int gamma;
    int sharpness;
};

struct video_cap {
    uint8_t desc[32];
    uint32_t version;
};

struct video_ctrl {
    uint32_t cmd;
    uint32_t val;
};

#define VIDCAP_GET_CAP         _IOWR('V',  0, struct video_cap)
#define VIDCAP_SET_CTRL        _IOWR('V',  1, struct video_ctrl)
#define VIDCAP_SET_CONF        _IOWR('V',  2, struct avcap_config)
#define VIDCAP_GET_LUMA        _IOWR('V',  3, int *)
#define VIDCAP_SET_LUMA        _IOWR('V',  4, int)
#define VIDCAP_GET_CTRST       _IOWR('V',  5, int *)
#define VIDCAP_SET_CTRST       _IOWR('V',  6, int)
#define VIDCAP_GET_SAT         _IOWR('V',  7, int *)
#define VIDCAP_SET_SAT         _IOWR('V',  8, int)
#define VIDCAP_GET_HUE         _IOWR('V',  9, int *)
#define VIDCAP_SET_HUE         _IOWR('V', 10, int)
#define VIDCAP_GET_AWB         _IOWR('V', 11, int *)
#define VIDCAP_SET_AWB         _IOWR('V', 12, int)
#define VIDCAP_GET_GAMMA       _IOWR('V', 13, int *)
#define VIDCAP_SET_GAMMA       _IOWR('V', 14, int)
#define VIDCAP_GET_SHARP       _IOWR('V', 15, int *)
#define VIDCAP_SET_SHARP       _IOWR('V', 16, int)




#ifdef __cplusplus
}
#endif
#endif
