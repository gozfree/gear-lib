/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    patch.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-29 14:25
 * updated: 2016-07-29 14:25
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include "patch.h"

bool decodeQtLanguageCode( uint16_t i_language_code, char *psz_iso,
                                  bool *b_mactables )
{
    static const char * psz_qt_to_iso639_2T_lower =
            "eng"    "fra"    "deu"    "ita"    "nld"
            "swe"    "spa"    "dan"    "por"    "nor" /* 5-9 */
            "heb"    "jpn"    "ara"    "fin"    "gre"
            "isl"    "mlt"    "tur"    "hrv"    "zho" /* 15-19 */
            "urd"    "hin"    "tha"    "kor"    "lit"
            "pol"    "hun"    "est"    "lav"    "sme" /* 25-29 */
            "fao"    "fas"    "rus"    "zho"    "nld" /* nld==flemish */
            "gle"    "sqi"    "ron"    "ces"    "slk" /* 35-39 */
            "slv"    "yid"    "srp"    "mkd"    "bul"
            "ukr"    "bel"    "uzb"    "kaz"    "aze" /* 45-49 */
            "aze"    "hye"    "kat"    "mol"    "kir"
            "tgk"    "tuk"    "mon"    "mon"    "pus" /* 55-59 */
            "kur"    "kas"    "snd"    "bod"    "nep"
            "san"    "mar"    "ben"    "asm"    "guj" /* 65-69 */
            "pan"    "ori"    "mal"    "kan"    "tam"
            "tel"    "sin"    "mya"    "khm"    "lao" /* 75-79 */
            "vie"    "ind"    "tgl"    "msa"    "msa"
            "amh"    "tir"    "orm"    "som"    "swa" /* 85-89 */
            "kin"    "run"    "nya"    "mlg"    "epo" /* 90-94 */
            ;

    static const char * psz_qt_to_iso639_2T_upper =
            "cym"    "eus"    "cat"    "lat"    "que" /* 128-132 */
            "grn"    "aym"    "tat"    "uig"    "dzo"
            "jaw"    "sun"    "glg"    "afr"    "bre" /* 138-142 */
            "iku"    "gla"    "glv"    "gle"    "ton"
            "gre"                                     /* 148 */
            ;

    if ( i_language_code < 0x400 || i_language_code == 0x7FFF )
    {
        const void *p_data;
        *b_mactables = true;
        if ( i_language_code <= 94 )
        {
            p_data = psz_qt_to_iso639_2T_lower + i_language_code * 3;
        }
        else if ( i_language_code >= 128 && i_language_code <= 148 )
        {
            i_language_code -= 128;
            p_data = psz_qt_to_iso639_2T_upper + i_language_code * 3;
        }
        else
            return false;
        memcpy( psz_iso, p_data, 3 );
    }
    else
    {
        *b_mactables = false;
        /*
         * to build code: ( ( 'f' - 0x60 ) << 10 ) | ( ('r' - 0x60) << 5 ) | ('a' - 0x60)
         */
        if( i_language_code == 0x55C4 ) /* "und" */
        {
            memset( psz_iso, 0, 3 );
            return false;
        }

        for( unsigned i = 0; i < 3; i++ )
            psz_iso[i] = ( ( i_language_code >> ( (2-i)*5 ) )&0x1f ) + 0x60;
    }
    return true;
}
void* file_open(stream_t *stream_s, const char* filename, int mode)
{
   FILE* file = NULL;
   const char* mode_fopen = NULL;
   if ((mode & MODE_READWRITEFILTER) == MODE_READ)
      mode_fopen = "rb";
   else
      if (mode & MODE_EXISTING)
         mode_fopen = "r+b";
      else
         if (mode & MODE_CREATE)
            mode_fopen = "wb";
   if ((filename != NULL) && (mode_fopen != NULL))
      file = fopen(filename, mode_fopen);
   stream_s->opaque = (void*)file;

   return file;
}

int file_read(stream_t *stream_s, void* buf, int size)
{
   FILE* file = (FILE*)stream_s->opaque;
   return fread(buf, 1, size, file);
}

int file_write(stream_t *stream_s, void *buf, int size)
{
   FILE* file = (FILE*)stream_s->opaque;
   return fwrite(buf, 1, size, file);
}

int file_peek(stream_t *stream_s, void* buf, int size)
{
   uint32_t offset = file_tell(stream_s);
   int ret = file_read(stream_s, buf, size);
   file_seek(stream_s, offset, SEEK_SET);
   return ret;
}

uint64_t file_seek(stream_t *stream_s, int64_t offset, int whence)
{
   FILE* file = (FILE*)stream_s->opaque;
   return fseek(file, offset, whence);
}

uint64_t file_tell(stream_t *stream_s)
{
   FILE* file = (FILE*)stream_s->opaque;
   return ftell(file);
}

int file_close(stream_t *stream_s)
{
   FILE* file = (FILE*)stream_s->opaque;
   return fclose(file);
}

stream_t* create_file_stream()
{
   stream_t* s = malloc(sizeof(stream_t));
   s->open = file_open;
   s->read = file_read;
   s->write = file_write;
   s->peek = file_peek;
   s->seek = file_seek;
   s->tell = file_tell;
   s->close = file_close;
   return s;
}

void destory_file_stream(stream_t* stream_s)
{
   free(stream_s);
}

stream_t* create_buf_file_stream()
{
   buf_stream_t* s = malloc(sizeof(buf_stream_t));
   stream_t* file_s = create_file_stream();
   s->s = *file_s;
   destory_file_stream(file_s);
   s->s.read = buf_file_read;
   s->s.write = buf_file_write;
   s->s.peek = buf_file_peek;
   s->s.seek = buf_file_seek;
   s->s.close = buf_file_close;
   s->read_buf_s.buf = malloc(READ_BUFFER_SIZE);
   s->read_buf_s.bufsize = -1;
   s->read_buf_s.offset = 0;
   s->write_buf_s.buf = malloc(WRITE_BUFFER_SIZE);
   s->write_buf_s.bufsize = 0;
   s->write_buf_s.offset = 0;
   s->offset = 0;
   return (stream_t*)s;
}

void destory_buf_file_stream(stream_t* stream_s)
{
   buf_stream_t* s = (buf_stream_t*)stream_s;
   free(s->read_buf_s.buf);
   free(s->write_buf_s.buf);
   free(stream_s);
}

int buf_file_read(stream_t *stream_s, void* buf, int size)
{
   buf_stream_t* s = (buf_stream_t*)stream_s;
   int length = 0, remainder = 0, position = 0;

   while (size > 0)
   {
      int read_bytes = 0;
      if (s->read_buf_s.offset > s->offset || 
         s->read_buf_s.offset + s->read_buf_s.bufsize <= s->offset ||
         s->read_buf_s.bufsize == -1)
      {
         int read_bytes, ret;
         // 先定位到offset, 再进行读操作.
         ret = file_seek(stream_s, s->offset, SEEK_SET);
         if (ret != 0)
         {
            assert(0);
            return ret;       // ERROR!!!
         }
         read_bytes = file_read(stream_s, s->read_buf_s.buf, READ_BUFFER_SIZE);
         if (read_bytes < 0)
            return read_bytes; // ERROR!!!
         if (read_bytes == 0)
            return length;
         if (s->read_buf_s.bufsize == -1)
            s->read_buf_s.offset = s->offset;
         else
            s->read_buf_s.offset += s->read_buf_s.bufsize;
         s->read_buf_s.bufsize = read_bytes;
      }

      position = s->offset - s->read_buf_s.offset;
      remainder = s->read_buf_s.bufsize - position;
      read_bytes = min(size, remainder);

      memcpy((char*)buf + length, 
         (char*)s->read_buf_s.buf + position, read_bytes);

      length += read_bytes;
      position += read_bytes;
      size -= read_bytes;
      remainder -= read_bytes;
      s->offset += read_bytes;
   }

   return length;
}

int buf_file_write(stream_t *stream_s, void *buf, int size)
{
   buf_stream_t* s = (buf_stream_t*)stream_s;
   int write_bytes = 0;

   if (s->write_buf_s.offset + s->write_buf_s.bufsize == s->offset &&
      s->write_buf_s.bufsize + size < WRITE_BUFFER_SIZE)
   {
      memcpy((char*)s->write_buf_s.buf + s->write_buf_s.bufsize, buf, size);
      s->write_buf_s.bufsize += size;
      s->offset += size;
      return size;
   }

   if (s->write_buf_s.bufsize != 0)
   {
      int ret = file_seek(stream_s, s->write_buf_s.offset, SEEK_SET);
      if (ret != 0)
         return ret;
      write_bytes = 0;
      while (write_bytes != s->write_buf_s.bufsize)
      {
         int n = file_write(stream_s, (char*)s->write_buf_s.buf + write_bytes, 
            s->write_buf_s.bufsize - write_bytes);
         if (n < 0)
            return n;
         write_bytes += n;
      }
      s->write_buf_s.bufsize = 0;
   }

   if (size > WRITE_BUFFER_SIZE)
   {
      int ret = file_seek(stream_s, s->offset, SEEK_SET);
      if (ret != 0)
         return ret;
      write_bytes = 0;
      while (write_bytes != size)
      {
         int n = file_write(stream_s, (char*)buf + write_bytes, size - write_bytes);
         if (n < 0)
            return n;
         write_bytes += n;
      }
      s->offset += size;
      return write_bytes;
   }
   else
   {
      memcpy(s->write_buf_s.buf, buf, size);
      s->write_buf_s.offset = s->offset;
      s->offset += size;
      s->write_buf_s.bufsize = size;
      return size;
   }
}

int buf_file_peek(stream_t *stream_s, void* buf, int size)
{
   buf_stream_t* s = (buf_stream_t*)stream_s;
   int ret = 0;
   int len = 0;

   ret = file_seek(stream_s, s->offset, SEEK_SET);
   if (ret != 0)
      return ret;

   len = file_read(stream_s, buf, size);
   ret = file_seek(stream_s, s->offset, SEEK_SET);
   if (ret != 0)
      return ret;

   return len;
}

uint64_t buf_file_seek(stream_t *stream_s, int64_t offset, int whence)
{
   buf_stream_t* s = (buf_stream_t*)stream_s;
   uint64_t ret = 0;
   ret = file_seek(stream_s, s->offset, SEEK_SET);
   if (ret != 0)
      return ret;
   ret = file_seek(stream_s, offset, whence);
   if (ret != 0)
      return ret;
   s->offset = file_tell(stream_s);
   if (s->read_buf_s.offset > s->offset || 
      s->read_buf_s.offset + s->read_buf_s.bufsize <= s->offset)
      s->read_buf_s.bufsize = -1;
   else
      s->read_buf_s.offset = s->offset;
   return ret;
}

int buf_file_close(stream_t *stream_s)
{
   buf_stream_t* s = (buf_stream_t*)stream_s;
   int write_bytes = 0;

   if (s->write_buf_s.bufsize != 0)
   {
      int ret = file_seek(stream_s, s->write_buf_s.offset, SEEK_SET);
      if (ret != 0)
         return ret;
      write_bytes = 0;
      while (write_bytes != s->write_buf_s.bufsize)
      {
         int n = file_write(stream_s, (char*)s->write_buf_s.buf + write_bytes, 
            s->write_buf_s.bufsize - write_bytes);
         if (n < 0)
            return n;
         write_bytes += n;
      }
      s->write_buf_s.bufsize = 0;
   }

   return file_close(stream_s);
}



uint16_t Swap16(uint16_t x)
{
   return ((x<<8)|(x>>8));
}

uint32_t Swap32(uint32_t x)
{
   return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}

uint64_t Swap64(uint64_t x)
{
   uint32_t hi, lo;

   /* Separate into high and low 32-bit values and swap them */
   lo = (uint32_t)(x & 0xFFFFFFFF);
   x >>= 32;
   hi = (uint32_t)(x & 0xFFFFFFFF);
   x = Swap32(lo);
   x <<= 32;
   x |= Swap32(hi);
   return(x);
}

uint16_t read_le16(stream_t *src)
{
   uint16_t value;

   stream_Read(src, &value, sizeof(value));
   return(SwapLE16(value));
}

uint16_t read_be16(stream_t *src)
{
   uint16_t value;

   stream_Read(src, &value, sizeof(value));
   return(SwapBE16(value));
}

uint32_t read_le32(stream_t *src)
{
   uint32_t value;

   stream_Read(src, &value, sizeof(value));
   return(SwapLE32(value));
}

uint32_t read_be32(stream_t *src)
{
   uint32_t value;

   stream_Read(src, &value, sizeof(value));
   return(SwapBE32(value));
}

uint64_t read_le64(stream_t *src)
{
   uint64_t value;

   stream_Read(src, &value, sizeof(value));
   return(SwapLE64(value));
}

uint64_t read_be64(stream_t *src)
{
   uint64_t value;

   stream_Read(src, &value, sizeof(value));
   return(SwapBE64(value));
}

int write_le16(stream_t *dst, uint16_t value)
{
   value = SwapLE16(value);
   return(stream_write(dst, &value, sizeof(value)));
}

int write_be16(stream_t *dst, uint16_t value)
{
   value = SwapBE16(value);
   return(stream_write(dst, &value, sizeof(value)));
}

int write_le32(stream_t *dst, uint32_t value)
{
   value = SwapLE32(value);
   return(stream_write(dst, &value, sizeof(value)));
}

int write_be32(stream_t *dst, uint32_t value)
{
   value = SwapBE32(value);
   return(stream_write(dst, &value, sizeof(value)));
}

int write_le64(stream_t *dst, uint64_t value)
{
   value = SwapLE64(value);
   return(stream_write(dst, &value, sizeof(value)));
}

int write_be64(stream_t *dst, uint64_t value)
{
   value = SwapBE64(value);
   return(stream_write(dst, &value, sizeof(value)));
}
