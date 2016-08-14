/******************************************************************************
 * Copyright (C) 2014-2016
 * file:    libmp4parser-patch.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-08-12 00:05
 * updated: 2016-08-12 00:05
 ******************************************************************************/

#ifndef __LIBMP4PARSER_PATCH_H__
#define __LIBMP4PARSER_PATCH_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#endif
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define typeof __typeof__


/* redefine bool to fix complie error */
#ifndef bool
#define bool uint8_t
#define true    1
#define false   0
#endif


/* define from vlc */
#ifdef WORDS_BIGENDIAN
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)d) | ( ((uint32_t)c) << 8 ) \
           | ( ((uint32_t)b) << 16 ) | ( ((uint32_t)a) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(b) | ( (uint16_t)(a) << 8 ) )

#else
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(a) | ( (uint16_t)(b) << 8 ) )

#endif

/* implement stream  */
typedef struct stream {
    void *(*open)(struct stream *stream_s, const char *filename, int mode);
    int (*read)(struct stream *stream_s, void *buf, int size);
    int (*write)(struct stream *stream_s, void *buf, int size);
    int (*peek)(struct stream *stream_s, const uint8_t **buf, int size);
    uint64_t (*seek)(struct stream *stream_s, int64_t offset, int whence);
    int64_t (*tell)(struct stream *stream_s);
    int64_t (*size)(struct stream *stream_s);
    int (*close)(struct stream *stream_s);
    void *opaque;
    void **priv_buf;//store peek malloc buffer
    int priv_buf_num;
} stream_t;

#define MODE_READ             (1)
#define MODE_WRITE            (2)
#define MODE_READWRITEFILTER  (3)
#define MODE_EXISTING         (4)
#define MODE_CREATE           (8)

stream_t* create_file_stream();
void destory_file_stream(stream_t* stream_s);
#define stream_open(s, filename, mode) ((stream_t*)s)->open(((stream_t*)s), filename, mode)
#define stream_close(s) ((stream_t*)s)->close(((stream_t*)s))

#ifdef __cplusplus
}
#endif
#endif
