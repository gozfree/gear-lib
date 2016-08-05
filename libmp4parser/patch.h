/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    patch.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-29 14:24
 * updated: 2016-07-29 14:24
 ******************************************************************************/
#ifndef __STREAM_H__
#define __STREAM_H__

#include <string.h>
#include <inttypes.h>
#include <stdint.h>

typedef uint32_t vlc_fourcc_t;
typedef uint8_t bool;
#define true    1
#define false   0

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

#ifdef __GNUC__
#   define likely(p)   __builtin_expect(!!(p), 1)
#   define unlikely(p) __builtin_expect(!!(p), 0)
#else
#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))
#endif

#define min(a, b)           ((a) > (b) ? (b) : (a))
#define max(a, b)           ((a) > (b) ? (a) : (b))

#define FREENULL(a) do { free( a ); a = NULL; } while(0)


#define msg_Dbg(fmt, ...) printf(fmt, __VA_ARGS__)
#define msg_Warn(fmt, ...) printf(fmt, __VA_ARGS__)
#define msg_Err(fmt, ...) printf(fmt, __VA_ARGS__)

#define VLC_UNUSED(x) (void)(x)
#   define __MIN(a, b)   ( ((a) < (b)) ? (a) : (b) )
#   define __MAX(a, b)   ( ((a) > (b)) ? (a) : (b) )


#ifdef __GNUC__
# define VLC_GCC_VERSION(maj,min) \
    ((__GNUC__ > (maj)) || (__GNUC__ == (maj) && __GNUC_MINOR__ >= (min)))
#else
# define VLC_GCC_VERSION(maj,min) (0)
#endif

static inline uint16_t bswap16 (uint16_t x)
{
    return (x << 8) | (x >> 8);
}

/** Byte swap (32 bits) */
static inline uint32_t bswap32 (uint32_t x)
{
#if VLC_GCC_VERSION(4,3) || defined(__clang__)
    return __builtin_bswap32 (x);
#else
    return ((x & 0x000000FF) << 24)
         | ((x & 0x0000FF00) <<  8)
         | ((x & 0x00FF0000) >>  8)
         | ((x & 0xFF000000) >> 24);
#endif
}

/** Byte swap (64 bits) */
static inline uint64_t bswap64 (uint64_t x)
{
#if VLC_GCC_VERSION(4,3) || defined(__clang__)
    return __builtin_bswap64 (x);
#elif !defined (__cplusplus)
    return ((x & 0x00000000000000FF) << 56)
         | ((x & 0x000000000000FF00) << 40)
         | ((x & 0x0000000000FF0000) << 24)
         | ((x & 0x00000000FF000000) <<  8)
         | ((x & 0x000000FF00000000) >>  8)
         | ((x & 0x0000FF0000000000) >> 24)
         | ((x & 0x00FF000000000000) >> 40)
         | ((x & 0xFF00000000000000) >> 56);
#else
    return ((x & 0x00000000000000FFULL) << 56)
         | ((x & 0x000000000000FF00ULL) << 40)
         | ((x & 0x0000000000FF0000ULL) << 24)
         | ((x & 0x00000000FF000000ULL) <<  8)
         | ((x & 0x000000FF00000000ULL) >>  8)
         | ((x & 0x0000FF0000000000ULL) >> 24)
         | ((x & 0x00FF000000000000ULL) >> 40)
         | ((x & 0xFF00000000000000ULL) >> 56);
#endif
}

#ifdef WORDS_BIGENDIAN
# define hton16(i) ((uint16_t)(i))
# define hton32(i) ((uint32_t)(i))
# define hton64(i) ((uint64_t)(i))
#else
# define hton16(i) bswap16(i)
# define hton32(i) bswap32(i)
# define hton64(i) bswap64(i)
#endif
#define ntoh16(i) hton16(i)
#define ntoh32(i) hton32(i)
#define ntoh64(i) hton64(i)


/** Reads 16 bits in network byte order */
static inline uint16_t U16_AT (const void *p)
{
    uint16_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh16 (x);
}

/** Reads 32 bits in network byte order */
static inline uint32_t U32_AT (const void *p)
{
    uint32_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh32 (x);
}

/** Reads 64 bits in network byte order */
static inline uint64_t U64_AT (const void *p)
{
    uint64_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh64 (x);
}

#define GetWBE(p)  U16_AT(p)
#define GetDWBE(p) U32_AT(p)
#define GetQWBE(p) U64_AT(p)

// 打开方式.
#define MODE_READ             (1)
#define MODE_WRITE            (2)
#define MODE_READWRITEFILTER  (3)
#define MODE_EXISTING         (4)
#define MODE_CREATE           (8)

// 读写结构定义.
typedef struct stream {
   void* (*open)(struct stream *stream_s, const char* filename, int mode);
   int (*read)(struct stream *stream_s, void* buf, int size);
   int (*write)(struct stream *stream_s, void *buf, int size);
   int (*peek)(struct stream *stream_s, void* buf, int size);
   uint64_t (*seek)(struct stream *stream_s, int64_t offset, int whence);
   uint64_t (*tell)(struct stream *stream_s);
   uint64_t (*size)(struct stream *stream_s);
   int (*close)(struct stream *stream_s);
   void* opaque;
} stream_t;

// 读写宏定义.
#define stream_open(s, filename, mode) ((stream_t*)s)->open(((stream_t*)s), filename, mode)
#define stream_Read(s, buf, size) ((stream_t*)s)->read(((stream_t*)s), buf, size)
#define stream_write(s, buf, size) ((stream_t*)s)->write(((stream_t*)s), buf, size)
#define stream_Peek(s, buf, size) ((stream_t*)s)->peek(((stream_t*)s), *buf, size)
#define stream_Seek(s, offset) ((stream_t*)s)->seek(((stream_t*)s), offset, SEEK_SET)
#define stream_Tell(s) ((stream_t*)s)->tell(((stream_t*)s))
#define stream_close(s) ((stream_t*)s)->close(((stream_t*)s))
#define stream_Size(s) ((stream_t*)s)->size(((stream_t*)s))

// 文件读写.
void* file_open(stream_t *stream_s, const char* filename, int mode);
int file_read(stream_t *stream_s, void* buf, int size);
int file_write(stream_t *stream_s, void *buf, int size);
int file_peek(stream_t *stream_s, void* buf, int size);
uint64_t file_seek(stream_t *stream_s, int64_t offset, int whence);
uint64_t file_tell(stream_t *stream_s);
int file_close(stream_t *stream_s);

// 创建文件读写流.
stream_t* create_file_stream();
void destory_file_stream(stream_t* stream_s);


// 一个带读写缓冲的文件stream实现.
#define READ_BUFFER_SIZE   10485760
#define WRITE_BUFFER_SIZE  10485760

typedef struct buf_stream {
   stream_t s;

   struct read_buf {
      void* buf;        // 读缓冲.
      int64_t bufsize;  // 缓冲大小.
      int64_t offset;   // 在文件中的offset.
   } read_buf_s;

   struct write_buf {
      void* buf;        // 写缓冲.
      int64_t bufsize;  // 缓冲大小.
      int64_t offset;   // 写文件的offset.
   } write_buf_s;

   uint64_t offset;     // 当前读写指针在文件中的offset.

} buf_stream_t;

// 带缓冲的读写实现.
int buf_file_read(stream_t *stream_s, void* buf, int size);
int buf_file_write(stream_t *stream_s, void *buf, int size);
int buf_file_peek(stream_t *stream_s, void* buf, int size);
uint64_t buf_file_seek(stream_t *stream_s, int64_t offset, int whence);
int buf_file_close(stream_t *stream_s);

// 创建带缓冲的文件读写流.
stream_t* create_buf_file_stream();
void destory_buf_file_stream(stream_t* stream_s);

// 大小端读写操作实现.
//#define LIL_ENDIAN	1234
//#define BIG_ENDIAN	4321
/* #define BYTEORDER    1234 ? 让下面宏根据系统进行判断大小端. */

#ifndef BYTEORDER
#if defined(__hppa__) || \
   defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
   (defined(__MIPS__) && defined(__MISPEB__)) || \
   defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
   defined(__sparc__)
#define BYTEORDER	BIG_ENDIAN
#else
#define BYTEORDER	LIL_ENDIAN
#endif
#endif /* !BYTEORDER */

uint16_t Swap16(uint16_t x);
uint32_t Swap32(uint32_t x);
uint64_t Swap64(uint64_t x);

#if BYTEORDER == LIL_ENDIAN
#define SwapLE16(X)	(X)
#define SwapLE32(X)	(X)
#define SwapLE64(X)	(X)
#define SwapBE16(X)	Swap16(X)
#define SwapBE32(X)	Swap32(X)
#define SwapBE64(X)	Swap64(X)
#else
#define SwapLE16(X)	Swap16(X)
#define SwapLE32(X)	Swap32(X)
#define SwapLE64(X)	Swap64(X)
#define SwapBE16(X)	(X)
#define SwapBE32(X)	(X)
#define SwapBE64(X)	(X)
#endif

uint16_t read_le16(stream_t *src);
uint16_t read_be16(stream_t *src);
uint32_t read_le32(stream_t *src);
uint32_t read_be32(stream_t *src);
uint64_t read_le64(stream_t *src);
uint64_t read_be64(stream_t *src);

int write_le16(stream_t *dst, uint16_t value);
int write_be16(stream_t *dst, uint16_t value);
int write_le32(stream_t *dst, uint32_t value);
int write_be32(stream_t *dst, uint32_t value);
int write_le64(stream_t *dst, uint64_t value);
int write_be64(stream_t *dst, uint64_t value);

bool decodeQtLanguageCode( uint16_t i_language_code, char *psz_iso,
                                  bool *b_mactables );

#endif
