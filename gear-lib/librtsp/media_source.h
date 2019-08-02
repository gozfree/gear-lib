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
#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_NAME_LEN     (128)
#define DESCRIPTION_LEN     (128)
#define SDP_LEN_MAX         8192

typedef struct media_source {
    char name[STREAM_NAME_LEN];
    char info[DESCRIPTION_LEN];
    char sdp[SDP_LEN_MAX];
    struct timeval tm_create;
    int (*sdp_generate)(struct media_source *ms);
    int (*open)(struct media_source *ms, const char *uri);
    int (*read)(struct media_source *ms, void **data, size_t *len);
    int (*write)(struct media_source *ms, void *data, size_t len);
    void (*close)(struct media_source *ms);
    int (*get_frame)();
    void *opaque;
    bool is_active;
    struct media_source *next;
} media_source_t;

void media_source_register_all();
struct media_source *rtsp_media_source_lookup(char *name);

#ifdef __cplusplus
}
#endif
#endif
