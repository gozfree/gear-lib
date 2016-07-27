/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    stream.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-27 18:22
 * updated: 2016-07-27 18:22
 ******************************************************************************/
#ifndef __STREAM_H__
#define __STREAM_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if 0
#ifndef _STDINT_H
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
//typedef unsigned long size_t;
#endif
#endif

#define MODE_READ             (1)
#define MODE_WRITE            (2)
#define MODE_READWRITEFILTER  (3)
#define MODE_EXISTING         (4)
#define MODE_CREATE           (8)
typedef struct stream {
   void* (*open)(struct stream *stream_s, const char* filename, int mode);
   int (*read)(struct stream *stream_s, void* buf, int size);
   int (*write)(struct stream *stream_s, void *buf, int size);
   int (*peek)(struct stream *stream_s, void* buf, int size);
   uint64_t (*seek)(struct stream *stream_s, int64_t offset, int whence);
   uint64_t (*tell)(struct stream *stream_s);
   int (*close)(struct stream *stream_s);
   void* opaque;
} stream_t;

#define stream_open(s, filename, mode) ((stream_t*)s)->open(((stream_t*)s), filename, mode)
#define stream_read(s, buf, size) ((stream_t*)s)->read(((stream_t*)s), buf, size)
#define stream_write(s, buf, size) ((stream_t*)s)->write(((stream_t*)s), buf, size)
#define stream_peek(s, buf, size) ((stream_t*)s)->peek(((stream_t*)s), buf, size)
#define stream_seek(s, offset, whence) ((stream_t*)s)->seek(((stream_t*)s), offset, whence)
#define stream_tell(s) ((stream_t*)s)->tell(((stream_t*)s))
#define stream_close(s) ((stream_t*)s)->close(((stream_t*)s))

void* sfile_open(stream_t *stream_s, const char* filename, int mode);
int sfile_read(stream_t *stream_s, void* buf, int size);
int sfile_write(stream_t *stream_s, void *buf, int size);
int sfile_peek(stream_t *stream_s, void* buf, int size);
uint64_t sfile_seek(stream_t *stream_s, int64_t offset, int whence);
uint64_t sfile_tell(stream_t *stream_s);
int sfile_close(stream_t *stream_s);

stream_t* create_file_stream();
void destory_file_stream(stream_t* stream_s);

typedef struct BUFFER {
	unsigned char *buf;
	unsigned char *begin_addr;
	unsigned long offset;
	unsigned long filesize;
}BUFFER_t;

//read data form buffer,not binary file
void* buffer_open(stream_t *stream_s, BUFFER_t *buffer);
int buffer_read(stream_t *stream_s, void* buf, int size);
int buffer_write(stream_t *stream_s, void *buf, int size);
int buffer_peek(stream_t *stream_s, void* buf, int size);
uint64_t buffer_seek(stream_t *stream_s, int64_t offset, int whence);
uint64_t buffer_tell(stream_t *stream_s);
int buffer_close(stream_t *stream_s);

stream_t* create_buffer_stream();
void destory_buffer_stream(stream_t* stream_s);

#define READ_BUFFER_SIZE   10485760
#define WRITE_BUFFER_SIZE  10485760

typedef struct buf_stream {
   stream_t s;

   struct read_buf {
      void* buf;        // ������.
      int64_t bufsize;  // �����С.
      int64_t offset;   // ���ļ��е�offset.
   } read_buf_s;

   struct write_buf {
      void* buf;        // д����.
      int64_t bufsize;  // �����С.
      int64_t offset;   // д�ļ���offset.
   } write_buf_s;

   uint64_t offset;     // ��ǰ��дָ�����ļ��е�offset.

} buf_stream_t;

int buf_file_read(stream_t *stream_s, void* buf, int size);
int buf_file_write(stream_t *stream_s, void *buf, int size);
int buf_file_peek(stream_t *stream_s, void* buf, int size);
uint64_t buf_file_seek(stream_t *stream_s, int64_t offset, int whence);
int buf_file_close(stream_t *stream_s);

stream_t* create_buf_file_stream();
void destory_buf_file_stream(stream_t* stream_s);

//#define LIL_ENDIAN	1234
//#define BIG_ENDIAN	4321
/* #define BYTEORDER    1234 ? ���������ϵͳ�����жϴ�С��. */

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

#endif // __STREAM_H__
