/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    mp4.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-27 18:22
 * updated: 2016-07-27 18:22
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include "mp4.h"

static mp4_box_t *MP4_ReadBox(stream_t *s, mp4_box_t *father);
static mp4_box_t *MP4_ReadBoxFromBuffer(stream_t *s, mp4_box_t *father);
//#define MP4_VERBOSE
void *debug_malloc(size_t size, const char *file, int line, const char *func)
{
        void *p;

        p = malloc(size);
        printf("%s:%d:%s:malloc(%ld): p=0x%lx\n",
            file, line, func, size, (unsigned long)p);
        return p;
}

//#define malloc(s) debug_malloc(s, __FILE__, __LINE__, __func__)
void *debug_free(char *p)
{
        //printf("%s:%d:%s:free(0x%lx)\n", __FILE__, __LINE__,  __func__, (unsigned long)p);
            if(NULL!=(p))
            {
            	free(p) ;
                p = NULL;
            }

}

#if 0
#define free(p)  do {                                                                                   \
        printf("%s:%d:%s:free(0x%lx)\n", __FILE__, __LINE__,            \
            __func__, (unsigned long)p);                                                       \
        if (p) {                                                                                                     \
          free(p);                                                                                                 \
          p = NULL;                                                                                           \
       }                                                                                                               \
} while (0)
#endif

//#define free(p) debug_free(p)


#define msg_Dbg(x) do {} while (0)
#define msg_Warn(x) do {} while (0)
#define msg_Err(x) do {} while (0)

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#define max(a,b) a>b?a:b
#define min(a,b) a>b?b:a
#define FREENULL(a) do { free( a ); a = NULL; } while(0)

#define MP4_GETX_PRIVATE(dst, code, size) do { \
    if( (i_read) >= (size) ) { dst = (code); p_peek += (size); } \
    else { dst = 0; }   \
    i_read -= (size);   \
  } while(0)

static uint32_t mp4_box_headersize( mp4_box_t *p_box )
{
   return 8
      + ( p_box->i_shortsize == 1 ? 8 : 0 )
      + ( p_box->i_type == ATOM_uuid ? 16 : 0 );
}

static uint32_t Get24bBE( const uint8_t *p )
{
   return( ( p[0] <<16 ) + ( p[1] <<8 ) + p[2] );
}

#define MP4_GET1BYTE( dst )  MP4_GETX_PRIVATE( dst, *p_peek, 1 )
#define MP4_GET2BYTES( dst ) MP4_GETX_PRIVATE( dst, SwapBE16(*(uint16_t*)p_peek), 2 )
#define MP4_GET3BYTES( dst ) MP4_GETX_PRIVATE( dst, Get24bBE(p_peek), 3 )
#define MP4_GET4BYTES( dst ) MP4_GETX_PRIVATE( dst, SwapBE32(*(uint32_t*)p_peek), 4 )
#define MP4_GET8BYTES( dst ) MP4_GETX_PRIVATE( dst, SwapBE64(*(uint64_t*)p_peek), 8 )
#define MP4_GETFOURCC( dst ) MP4_GETX_PRIVATE( dst, \
                MP4_FOURCC(p_peek[0],p_peek[1],p_peek[2],p_peek[3]), 4)

#define MP4_GETVERSIONFLAGS( p_void ) \
    MP4_GET1BYTE( p_void->version ); \
    MP4_GET3BYTES( p_void->flags )

#define MP4_GETSTRINGZ( p_str )         \
    if( (i_read > 0) && (p_peek[0]) )   \
    {       \
        const int __i_copy__ = strnlen( (char*)p_peek, i_read-1 );  \
        p_str = malloc( __i_copy__+1 );               \
        if( p_str ) \
        { \
             memcpy( p_str, p_peek, __i_copy__ ); \
             p_str[__i_copy__] = 0; \
        } \
        p_peek += __i_copy__ + 1;   \
        i_read -= __i_copy__ + 1;   \
    }       \
    else    \
    {       \
        p_str = NULL; \
    }

#define MP4_READBOX_ENTER( mp4_box_data_TYPE_t ) \
    int64_t  i_read = p_box->i_size; \
    uint8_t *p_peek, *p_buff; \
    int i_actually_read; \
    if( !( p_peek = p_buff = malloc( i_read ) ) ) \
    { \
        return( 0 ); \
    } \
    i_actually_read = stream_read( p_stream, p_peek, i_read ); \
    if( i_actually_read < 0 || (int64_t)i_actually_read < i_read )\
    { \
        free( p_buff ); \
        return( 0 ); \
    } \
    p_peek += mp4_box_headersize( p_box ); \
    i_read -= mp4_box_headersize( p_box ); \
    if( !( p_box->data.p_data = calloc( 1, sizeof( mp4_box_data_TYPE_t ) ) ) ) \
    { \
        free( p_buff ); \
        return( 0 ); \
    }

#define MP4_READBOX_ENTERFromBuffer( mp4_box_data_TYPE_t ) \
    int64_t  i_read = p_box->i_size; \
    uint8_t *p_peek, *p_buff; \
    int i_actually_read; \
    if( !( p_peek = p_buff = malloc( i_read ) ) ) \
    { \
        return( 0 ); \
    } \
    i_actually_read = buffer_read( p_stream, p_peek, i_read ); \
    if( i_actually_read < 0 || (int64_t)i_actually_read < i_read )\
    { \
        free( p_buff ); \
        return( 0 ); \
    } \
    p_peek += mp4_box_headersize( p_box ); \
    i_read -= mp4_box_headersize( p_box ); \
    if( !( p_box->data.p_data = calloc( 1, sizeof( mp4_box_data_TYPE_t ) ) ) ) \
    { \
        free( p_buff ); \
        return( 0 ); \
    }

#define MP4_READBOX_EXIT( i_code ) \
    do \
    { \
        free( p_buff ); \
        if( i_read < 0 ) \
            printf( "Not enough data" ); \
        return( i_code ); \
    } while (0)


/* Some assumptions:
 * The input method HAS to be seekable
 */

/* This macro is used when we want to printf the box type
 * APPLE annotation box is :
 *  either 0xA9 + 24-bit ASCII text string (and 0xA9 isn't printable)
 *  either 32-bit ASCII text string
 */
#define MP4_BOX_TYPE_ASCII() ( ((char*)&p_box->i_type)[0] != (char)0xA9 )


static void CreateUUID( uuid_t *p_uuid, uint32_t i_fourcc )
{
   /* made by 0xXXXXXXXX-0011-0010-8000-00aa00389b71 where XXXXXXXX is the fourcc */
   /* FIXME implement this */
   (void)p_uuid;
   (void)i_fourcc;
}

static int drms_init( void *_p_drms, uint32_t i_type,
              uint8_t *p_info, uint32_t i_len )
{
   return 1;
}

static void GetUUID( uuid_t *p_uuid, const uint8_t *p_buff )
{
   memcpy( p_uuid, p_buff, 16 );
}

/* convert 16.16 fixed point to floating point */
static double conv_fx( int32_t fx ) {
   double fp = fx;
   fp /= 65536.0;
   return fp;
}

/* some functions for mp4 encoding of variables */
#ifdef MP4_VERBOSE
static void MP4_ConvertDate2Str( char *psz, uint64_t i_date )
{
   int i_day;
   int i_hour;
   int i_min;
   int i_sec;

   /* date begin at 1 jan 1904 */
   i_date += ((INT64_C(1904) * 365) + 17) * 24 * 60 * 60;

   i_day = i_date / ( 60*60*24);
   i_hour = ( i_date /( 60*60 ) ) % 60;
   i_min  = ( i_date / 60 ) % 60;
   i_sec =  i_date % 60;
   sprintf( psz, "%dd-%2.2dh:%2.2dm:%2.2ds", i_day, i_hour, i_min, i_sec );
}
#endif

/*****************************************************************************
 * MP4_ReadBoxCommon : Load only common parameters for all boxes
 *****************************************************************************
 * p_box need to be an already allocated mp4_box_t, and all data
 *  will only be peek not read
 *
 * RETURN : 0 if it fail, 1 otherwise
 *****************************************************************************/
int MP4_ReadBoxCommon( stream_t *p_stream, mp4_box_t *p_box )
{
   int      i_read;
   const uint8_t *p_buff = malloc(32);
   const uint8_t *p_peek = p_buff;

   if( ( ( i_read = stream_peek( p_stream, p_peek, 32 ) ) < 8 ) )
   {
      free(p_buff);
      return 0;
   }

   p_box->i_pos = stream_tell( p_stream );

   p_box->data.p_data = NULL;
   p_box->p_father = NULL;
   p_box->p_first  = NULL;
   p_box->p_last  = NULL;
   p_box->p_next   = NULL;

   MP4_GET4BYTES( p_box->i_shortsize );
   MP4_GETFOURCC( p_box->i_type );

   /* Now special case */

   if( p_box->i_shortsize == 1 )
   {
      /* get the true size on 64 bits */
      MP4_GET8BYTES( p_box->i_size );
   }
   else
   {
      p_box->i_size = p_box->i_shortsize;
      /* XXX size of 0 means that the box extends to end of file */
   }

   if( p_box->i_type == ATOM_uuid )
   {
      /* get extented type on 16 bytes */
      GetUUID( &p_box->i_uuid, p_peek );
      p_peek += 16; i_read -= 16;
   }
   else
   {
      CreateUUID( &p_box->i_uuid, p_box->i_type );
   }

   free(p_buff);

// #ifdef MP4_VERBOSE
//    if( p_box->i_size )
//    {
//       if MP4_BOX_TYPE_ASCII()
//           printf( "found Box: %4.4s size %"PRId64,
//          (char*)&p_box->i_type, p_box->i_size );
//       else
//           printf( "found Box: c%3.3s size %"PRId64,
//          (char*)&p_box->i_type+1, p_box->i_size );
//    }
// #endif

   return 1;
}

int MP4_ReadBoxCommonFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   int      i_read;
   const uint8_t *p_buff = malloc(32);
   const uint8_t *p_peek = p_buff;

   if( ( ( i_read = buffer_peek( p_stream, p_peek, 32 ) ) < 8 ) )
   {
      free(p_buff);
      return 0;
   }

   p_box->i_pos = buffer_tell( p_stream );

   p_box->data.p_data = NULL;
   p_box->p_father = NULL;
   p_box->p_first  = NULL;
   p_box->p_last  = NULL;
   p_box->p_next   = NULL;

   MP4_GET4BYTES( p_box->i_shortsize );
   MP4_GETFOURCC( p_box->i_type );

   /* Now special case */

   if( p_box->i_shortsize == 1 )
   {
      /* get the true size on 64 bits */
      MP4_GET8BYTES( p_box->i_size );
   }
   else
   {
      p_box->i_size = p_box->i_shortsize;
      /* XXX size of 0 means that the box extends to end of file */
   }

   if( p_box->i_type == ATOM_uuid )
   {
      /* get extented type on 16 bytes */
      GetUUID( &p_box->i_uuid, p_peek );
      p_peek += 16; i_read -= 16;
   }
   else
   {
      CreateUUID( &p_box->i_uuid, p_box->i_type );
   }

   free(p_buff);

// #ifdef MP4_VERBOSE
//    if( p_box->i_size )
//    {
//       if MP4_BOX_TYPE_ASCII()
//           printf( "found Box: %4.4s size %"PRId64,
//          (char*)&p_box->i_type, p_box->i_size );
//       else
//           printf( "found Box: c%3.3s size %"PRId64,
//          (char*)&p_box->i_type+1, p_box->i_size );
//    }
// #endif

   return 1;
}
/*****************************************************************************
 * MP4_NextBox : Go to the next box
 *****************************************************************************
 * if p_box == NULL, go to the next box in which we are( at the begining ).
 *****************************************************************************/
static int MP4_NextBox( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_t box;

   if( !p_box )
   {
      MP4_ReadBoxCommon( p_stream, &box );
      p_box = &box;
   }

   if( !p_box->i_size )
   {
      return 2; /* Box with infinite size */
   }

   if( p_box->p_father )
   {
      const int64_t i_box_end = p_box->i_size + p_box->i_pos;
      const int64_t i_father_end = p_box->p_father->i_size + p_box->p_father->i_pos;

      /* check if it's within p-father */
      if( i_box_end >= i_father_end )
      {
         if( i_box_end > i_father_end )
        	 printf(  "out of bound child" );
         return 0; /* out of bound */
      }
   }

   if (stream_seek( p_stream, p_box->i_size + p_box->i_pos, SEEK_SET ))
   {
      return 0;
   }

   return 1;
}

static int MP4_NextBoxFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_t box;

   if( !p_box )
   {
      MP4_ReadBoxCommonFromBuffer( p_stream, &box );
      p_box = &box;
   }

   if( !p_box->i_size )
   {
      return 2; /* Box with infinite size */
   }

   if( p_box->p_father )
   {
      const int64_t i_box_end = p_box->i_size + p_box->i_pos;
      const int64_t i_father_end = p_box->p_father->i_size + p_box->p_father->i_pos;

      /* check if it's within p-father */
      if( i_box_end >= i_father_end )
      {
         if( i_box_end > i_father_end )
        	 printf(  "out of bound child" );
         return 0; /* out of bound */
      } 
   }

   if (buffer_seek( p_stream, p_box->i_size, p_box->i_pos ))
   {
      return 0;
   }

   return 1;
}
/*****************************************************************************
 * For all known box a loader is given,
 *  XXX: all common struct have to be already read by MP4_ReadBoxCommon
 *       after called one of theses functions, file position is unknown
 *       you need to call MP4_GotoBox to go where you want
 *****************************************************************************/
static int MP4_ReadBoxContainerRaw( stream_t *p_stream, mp4_box_t *p_container )
{
   mp4_box_t *p_box;

   if( stream_tell( p_stream ) + 8 >
	   (int64_t)(p_container->i_pos + p_container->i_size) )
   {
      /* there is no box to load */
      return 0;
   }

   do
   {
      if( ( p_box = MP4_ReadBox( p_stream, p_container ) ) == NULL ) break;

      /* chain this box with the father and the other at same level */
      if( !p_container->p_first ) p_container->p_first = p_box;
      else p_container->p_last->p_next = p_box;
      p_container->p_last = p_box;

   } while( MP4_NextBox( p_stream, p_box ) == 1 );

   return 1;
}

static int MP4_ReadBoxContainerRawFromBuffer( stream_t *p_stream, mp4_box_t *p_container )
{
   mp4_box_t *p_box;

   if( buffer_tell( p_stream ) + 8 >
	   (int64_t)(p_container->i_pos + p_container->i_size) )
   {
      /* there is no box to load */
      return 0;
   }

   do
   {
      if( ( p_box = MP4_ReadBoxFromBuffer( p_stream, p_container ) ) == NULL ) break;
      /* chain this box with the father and the other at same level */
      if( !p_container->p_first ) p_container->p_first = p_box;
      else p_container->p_last->p_next = p_box;
      p_container->p_last = p_box;

   } while( MP4_NextBoxFromBuffer( p_stream, p_box ) == 1 );

   return 1;
}

static int MP4_ReadBoxContainer( stream_t *p_stream, mp4_box_t *p_container )
{
   if( p_container->i_size <= (size_t)mp4_box_headersize(p_container ) + 8 )
   {
      /* container is empty, 8 stand for the first header in this box */
      return 1;
   }

   /* enter box */
   stream_seek( p_stream, p_container->i_pos +
      mp4_box_headersize( p_container ), SEEK_SET );
 
   return MP4_ReadBoxContainerRaw( p_stream, p_container );
}

static int MP4_ReadBoxContainerFromBuffer( stream_t *p_stream, mp4_box_t *p_container )
{
   if( p_container->i_size <= (size_t)mp4_box_headersize(p_container ) + 8 )
   {
      /* container is empty, 8 stand for the first header in this box */
      return 1;
   }

   /* enter box */
   buffer_seek( p_stream, mp4_box_headersize( p_container ), p_container->i_pos );
 
   return MP4_ReadBoxContainerRawFromBuffer( p_stream, p_container );
}

static void MP4_FreeBox_Common( mp4_box_t *p_box )
{
   /* Up to now do nothing */
   (void)p_box;
}

static int MP4_ReadBoxSkip( stream_t *p_stream, mp4_box_t *p_box )
{
   /* XXX sometime moov is hiden in a free box */
   if( p_box->p_father &&
      p_box->p_father->i_type == ATOM_root &&
      p_box->i_type == ATOM_free )
   {
      const uint8_t *p_buff = malloc(44);
      const uint8_t *p_peek = p_buff;
      int     i_read;
      uint32_t i_fcc;

      i_read  = stream_peek( p_stream, p_peek, 44 );

      p_peek += mp4_box_headersize( p_box ) + 4;
      i_read -= mp4_box_headersize( p_box ) + 4;

      if( i_read >= 8 )
      {
         i_fcc = MP4_FOURCC( p_peek[0], p_peek[1], p_peek[2], p_peek[3] );

         if( i_fcc == ATOM_cmov || i_fcc == ATOM_mvhd )
         {
            printf( "detected moov hidden in a free box ..." );

            p_box->i_type = ATOM_foov;
            free(p_buff);
            return MP4_ReadBoxContainer( p_stream, p_box );
         }
      }
      free(p_buff);
   }

   /* Nothing to do */
// #ifdef MP4_VERBOSE
//    if MP4_BOX_TYPE_ASCII()
//        printf( "skip box: \"%4.4s\"", (char*)&p_box->i_type );
//    else
//        printf( "skip box: \"c%3.3s\"", (char*)&p_box->i_type+1 );
// #endif

   return 1;
}

static int MP4_ReadBoxSkipFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   /* XXX sometime moov is hiden in a free box */
   if( p_box->p_father &&
      p_box->p_father->i_type == ATOM_root &&
      p_box->i_type == ATOM_free )
   {
      const uint8_t *p_buff = malloc(44);
      const uint8_t *p_peek = p_buff;
      int     i_read;
      uint32_t i_fcc;

      i_read  = buffer_peek( p_stream, p_peek, 44 );

      p_peek += mp4_box_headersize( p_box ) + 4;
      i_read -= mp4_box_headersize( p_box ) + 4;

      if( i_read >= 8 )
      {
         i_fcc = MP4_FOURCC( p_peek[0], p_peek[1], p_peek[2], p_peek[3] );

         if( i_fcc == ATOM_cmov || i_fcc == ATOM_mvhd )
         {
            printf( "detected moov hidden in a free box ..." );

            p_box->i_type = ATOM_foov;
            free(p_buff);
            return MP4_ReadBoxContainerFromBuffer( p_stream, p_box );
         }
      }
      free(p_buff);
   }

   /* Nothing to do */
// #ifdef MP4_VERBOSE
//    if MP4_BOX_TYPE_ASCII()
//        printf( "skip box: \"%4.4s\"", (char*)&p_box->i_type );
//    else
//        printf( "skip box: \"c%3.3s\"", (char*)&p_box->i_type+1 );
// #endif

   return 1;
}

#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))

static int MP4_ReadBox_ftyp( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_ftyp_t );

   MP4_GETFOURCC( p_box->data.p_ftyp->major_brand );
   MP4_GET4BYTES( p_box->data.p_ftyp->minor_version );

   if( ( p_box->data.p_ftyp->compatible_brands_count = i_read / 4 ) )
   {
      unsigned int i = 0;
      uint32_t *tab = p_box->data.p_ftyp->compatible_brands =
         calloc( p_box->data.p_ftyp->compatible_brands_count, sizeof(uint32_t));

      if( unlikely( tab == NULL ) )
         MP4_READBOX_EXIT( 0 );

      for( i = 0; i < p_box->data.p_ftyp->compatible_brands_count; i++ )
      {
         MP4_GETFOURCC( tab[i] );
      }
   }
   else
   {
      p_box->data.p_ftyp->compatible_brands = NULL;
   }

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_ftypFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_ftyp_t );

   MP4_GETFOURCC( p_box->data.p_ftyp->major_brand );
   MP4_GET4BYTES( p_box->data.p_ftyp->minor_version );
  
   if( ( p_box->data.p_ftyp->compatible_brands_count = i_read / 4 ) )
   {
      unsigned int i = 0;
      uint32_t *tab = p_box->data.p_ftyp->compatible_brands =
         calloc( p_box->data.p_ftyp->compatible_brands_count, sizeof(uint32_t));

      if( unlikely( tab == NULL ) )
         MP4_READBOX_EXIT( 0 );

      for( i = 0; i < p_box->data.p_ftyp->compatible_brands_count; i++ )
      {
         MP4_GETFOURCC( tab[i] );
      }
   }
   else
   {
      p_box->data.p_ftyp->compatible_brands = NULL;
   }

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_ftyp( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_ftyp->compatible_brands );
}

static int MP4_ReadBox_mmpu( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int mmpu_buf;
   int i;
   MP4_READBOX_ENTER( mp4_box_data_mmpu_t );
   MP4_GETVERSIONFLAGS(p_box->data.p_mmpu);
   MP4_GET1BYTE(mmpu_buf);
   p_box->data.p_mmpu->is_complete=(mmpu_buf>>7)&0x01;
   p_box->data.p_mmpu->reserved=mmpu_buf&0x7F;
   MP4_GET4BYTES( p_box->data.p_mmpu->mpu_sequence_number );
   MP4_GETFOURCC( p_box->data.p_mmpu->asset_id_scheme );
   MP4_GET4BYTES( p_box->data.p_mmpu->asset_id_length );
   p_box->data.p_mmpu->asset_id_value=(char *)malloc(p_box->data.p_mmpu->asset_id_length);
   for (i=0;i<p_box->data.p_mmpu->asset_id_length;i++)
   {

	   MP4_GET1BYTE( p_box->data.p_mmpu->asset_id_value[i] );
   }



   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mmpuFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int mmpu_buf;
   int i;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_mmpu_t );
   MP4_GETVERSIONFLAGS(p_box->data.p_mmpu);
   MP4_GET1BYTE(mmpu_buf);
   p_box->data.p_mmpu->is_complete=(mmpu_buf>>7)&0x01;
   p_box->data.p_mmpu->reserved=mmpu_buf&0x7F;
   MP4_GET4BYTES( p_box->data.p_mmpu->mpu_sequence_number );
   MP4_GETFOURCC( p_box->data.p_mmpu->asset_id_scheme );
   MP4_GET4BYTES( p_box->data.p_mmpu->asset_id_length );
   p_box->data.p_mmpu->asset_id_value=(char *)malloc(p_box->data.p_mmpu->asset_id_length);
   for (i=0;i<p_box->data.p_mmpu->asset_id_length;i++)
   {

	   MP4_GET1BYTE( p_box->data.p_mmpu->asset_id_value[i] );
   }
  
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_tfdt( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_tfdt_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_tfdt );

       if( p_box->data.p_tfdt->version )
       {
           MP4_GET8BYTES( p_box->data.p_tfdt->baseMediaDecodeTime );

       }
       else
       {
           MP4_GET4BYTES( p_box->data.p_tfdt->baseMediaDecodeTime );

       }



   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_tfdtFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_tfdt_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_tfdt );

       if( p_box->data.p_tfdt->version )
       {
           MP4_GET8BYTES( p_box->data.p_tfdt->baseMediaDecodeTime );

       }
       else
       {
           MP4_GET4BYTES( p_box->data.p_tfdt->baseMediaDecodeTime );

       }



   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_mmpu( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_mmpu->asset_id_value );
}


static int MP4_ReadBox_mvhd(  stream_t *p_stream, mp4_box_t *p_box )
{
#ifdef MP4_VERBOSE
    char s_creation_time[128];
    char s_modification_time[128];
    char s_duration[128];
#endif
    unsigned int i = 0;
    MP4_READBOX_ENTER( mp4_box_data_mvhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_mvhd );

    if( p_box->data.p_mvhd->version )
    {
        MP4_GET8BYTES( p_box->data.p_mvhd->creation_time );
        MP4_GET8BYTES( p_box->data.p_mvhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mvhd->timescale );
        MP4_GET8BYTES( p_box->data.p_mvhd->duration );
    }
    else
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->creation_time );
        MP4_GET4BYTES( p_box->data.p_mvhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mvhd->timescale );
        MP4_GET4BYTES( p_box->data.p_mvhd->duration );
    }
    MP4_GET4BYTES( p_box->data.p_mvhd->rate );
    MP4_GET2BYTES( p_box->data.p_mvhd->volume );
    MP4_GET2BYTES( p_box->data.p_mvhd->reserved1 );


    for( i = 0; i < 2; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->reserved2[i] );
    }
    for( i = 0; i < 9; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->matrix[i] );
    }
    for( i = 0; i < 6; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->predefined[i] );
    }

    MP4_GET4BYTES( p_box->data.p_mvhd->next_track_id );


#ifdef MP4_VERBOSE
    MP4_ConvertDate2Str( s_creation_time, p_box->data.p_mvhd->creation_time );
    MP4_ConvertDate2Str( s_modification_time,
                         p_box->data.p_mvhd->modification_time );
    if( p_box->data.p_mvhd->rate )
    {
        MP4_ConvertDate2Str( s_duration,
                 p_box->data.p_mvhd->duration / p_box->data.p_mvhd->rate );
    }
    else
    {
        s_duration[0] = 0;
    }
     printf( "read box: \"mvhd\" creation %s modification %s time scale %d duration %s rate %f volume %f next track id %d",
                  s_creation_time,
                  s_modification_time,
                  (uint32_t)p_box->data.p_mvhd->timescale,
                  s_duration,
                  (float)p_box->data.p_mvhd->rate / (1<<16 ),
                  (float)p_box->data.p_mvhd->volume / 256 ,
                  (uint32_t)p_box->data.p_mvhd->next_track_id );
#endif
    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mvhdFromBuffer(  stream_t *p_stream, mp4_box_t *p_box )
{
#ifdef MP4_VERBOSE
    char s_creation_time[128];
    char s_modification_time[128];
    char s_duration[128];
#endif
    unsigned int i = 0;
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_mvhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_mvhd );

    if( p_box->data.p_mvhd->version )
    {
        MP4_GET8BYTES( p_box->data.p_mvhd->creation_time );
        MP4_GET8BYTES( p_box->data.p_mvhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mvhd->timescale );
        MP4_GET8BYTES( p_box->data.p_mvhd->duration );
    }
    else
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->creation_time );
        MP4_GET4BYTES( p_box->data.p_mvhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mvhd->timescale );
        MP4_GET4BYTES( p_box->data.p_mvhd->duration );
    }
    MP4_GET4BYTES( p_box->data.p_mvhd->rate );
    MP4_GET2BYTES( p_box->data.p_mvhd->volume );
    MP4_GET2BYTES( p_box->data.p_mvhd->reserved1 );


    for( i = 0; i < 2; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->reserved2[i] );
    }
    for( i = 0; i < 9; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->matrix[i] );
    }
    for( i = 0; i < 6; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_mvhd->predefined[i] );
    }

    MP4_GET4BYTES( p_box->data.p_mvhd->next_track_id );


#ifdef MP4_VERBOSE
    MP4_ConvertDate2Str( s_creation_time, p_box->data.p_mvhd->creation_time );
    MP4_ConvertDate2Str( s_modification_time,
                         p_box->data.p_mvhd->modification_time );
    if( p_box->data.p_mvhd->rate )
    {
        MP4_ConvertDate2Str( s_duration,
                 p_box->data.p_mvhd->duration / p_box->data.p_mvhd->rate );
    }
    else
    {
        s_duration[0] = 0;
    }
     printf( "read box: \"mvhd\" creation %s modification %s time scale %d duration %s rate %f volume %f next track id %d",
                  s_creation_time,
                  s_modification_time,
                  (uint32_t)p_box->data.p_mvhd->timescale,
                  s_duration,
                  (float)p_box->data.p_mvhd->rate / (1<<16 ),
                  (float)p_box->data.p_mvhd->volume / 256 ,
                  (uint32_t)p_box->data.p_mvhd->next_track_id );
#endif
    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mfhd(  stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_mfhd_t );

    MP4_GET4BYTES( p_box->data.p_mfhd->sequence_number );

#ifdef MP4_VERBOSE
     printf( "read box: \"mfhd\" sequence number %d",
                  p_box->data.p_mfhd->sequence_number );
#endif
    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mfhdFromBuffer(  stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_mfhd_t );

    MP4_GET4BYTES( p_box->data.p_mfhd->sequence_number );

#ifdef MP4_VERBOSE
     printf( "read box: \"mfhd\" sequence number %d",
                  p_box->data.p_mfhd->sequence_number );
#endif
    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_tfhd(  stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_tfhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_tfhd );

    MP4_GET4BYTES( p_box->data.p_tfhd->track_ID );

    if( p_box->data.p_tfhd->version == 0 )
    {
        if( p_box->data.p_tfhd->flags & MP4_TFHD_BASE_DATA_OFFSET )
            MP4_GET8BYTES( p_box->data.p_tfhd->base_data_offset );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_SAMPLE_DESC_INDEX )
            MP4_GET4BYTES( p_box->data.p_tfhd->sample_description_index );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_DFLT_SAMPLE_DURATION )
            MP4_GET4BYTES( p_box->data.p_tfhd->default_sample_duration );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_DFLT_SAMPLE_SIZE )
            MP4_GET4BYTES( p_box->data.p_tfhd->default_sample_size );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_DFLT_SAMPLE_FLAGS )
            MP4_GET4BYTES( p_box->data.p_tfhd->default_sample_flags );

#ifdef MP4_VERBOSE
        char psz_base[128] = "\0";
        char psz_desc[128] = "\0";
        char psz_dura[128] = "\0";
        char psz_size[128] = "\0";
        char psz_flag[128] = "\0";
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_BASE_DATA_OFFSET )
            snprintf(psz_base, sizeof(psz_base), "base offset %"PRId64, p_box->data.p_tfhd->base_data_offset);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_SAMPLE_DESC_INDEX )
            snprintf(psz_desc, sizeof(psz_desc), "sample description index %d", p_box->data.p_tfhd->sample_description_index);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_DFLT_SAMPLE_DURATION )
            snprintf(psz_dura, sizeof(psz_dura), "sample duration %d", p_box->data.p_tfhd->default_sample_duration);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_DFLT_SAMPLE_SIZE )
            snprintf(psz_size, sizeof(psz_size), "sample size %d", p_box->data.p_tfhd->default_sample_size);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_DFLT_SAMPLE_FLAGS )
            snprintf(psz_flag, sizeof(psz_flag), "sample flags 0x%x", p_box->data.p_tfhd->default_sample_flags);

         printf( "read box: \"tfhd\" version %d flags 0x%x track ID %d %s %s %s %s %s",
                    p_box->data.p_tfhd->i_version,
                    p_box->data.p_tfhd->i_flags,
                    p_box->data.p_tfhd->track_ID,
                    psz_base, psz_desc, psz_dura, psz_size, psz_flag );
#endif
    }

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_tfhdFromBuffer(  stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_tfhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_tfhd );

    MP4_GET4BYTES( p_box->data.p_tfhd->track_ID );

    if( p_box->data.p_tfhd->version == 0 )
    {
        if( p_box->data.p_tfhd->flags & MP4_TFHD_BASE_DATA_OFFSET )
            MP4_GET8BYTES( p_box->data.p_tfhd->base_data_offset );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_SAMPLE_DESC_INDEX )
            MP4_GET4BYTES( p_box->data.p_tfhd->sample_description_index );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_DFLT_SAMPLE_DURATION )
            MP4_GET4BYTES( p_box->data.p_tfhd->default_sample_duration );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_DFLT_SAMPLE_SIZE )
            MP4_GET4BYTES( p_box->data.p_tfhd->default_sample_size );
        if( p_box->data.p_tfhd->flags & MP4_TFHD_DFLT_SAMPLE_FLAGS )
            MP4_GET4BYTES( p_box->data.p_tfhd->default_sample_flags );

#ifdef MP4_VERBOSE
        char psz_base[128] = "\0";
        char psz_desc[128] = "\0";
        char psz_dura[128] = "\0";
        char psz_size[128] = "\0";
        char psz_flag[128] = "\0";
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_BASE_DATA_OFFSET )
            snprintf(psz_base, sizeof(psz_base), "base offset %"PRId64, p_box->data.p_tfhd->base_data_offset);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_SAMPLE_DESC_INDEX )
            snprintf(psz_desc, sizeof(psz_desc), "sample description index %d", p_box->data.p_tfhd->sample_description_index);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_DFLT_SAMPLE_DURATION )
            snprintf(psz_dura, sizeof(psz_dura), "sample duration %d", p_box->data.p_tfhd->default_sample_duration);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_DFLT_SAMPLE_SIZE )
            snprintf(psz_size, sizeof(psz_size), "sample size %d", p_box->data.p_tfhd->default_sample_size);
        if( p_box->data.p_tfhd->i_flags & MP4_TFHD_DFLT_SAMPLE_FLAGS )
            snprintf(psz_flag, sizeof(psz_flag), "sample flags 0x%x", p_box->data.p_tfhd->default_sample_flags);

         printf( "read box: \"tfhd\" version %d flags 0x%x track ID %d %s %s %s %s %s",
                    p_box->data.p_tfhd->i_version,
                    p_box->data.p_tfhd->i_flags,
                    p_box->data.p_tfhd->track_ID,
                    psz_base, psz_desc, psz_dura, psz_size, psz_flag );
#endif
    }

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_trun(  stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i;
    MP4_READBOX_ENTER( mp4_box_data_trun_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_trun );

    MP4_GET4BYTES( p_box->data.p_trun->sample_count );

    if( p_box->data.p_trun->flags & MP4_TRUN_DATA_OFFSET )
        MP4_GET4BYTES( p_box->data.p_trun->data_offset );
    if( p_box->data.p_trun->flags & MP4_TRUN_FIRST_FLAGS )
        MP4_GET4BYTES( p_box->data.p_trun->first_sample_flags );

    p_box->data.p_trun->samples =
      calloc( p_box->data.p_trun->sample_count, sizeof(mp4_descriptor_trun_sample_t) );
    if ( p_box->data.p_trun->samples == NULL )
        MP4_READBOX_EXIT( 0 );

    for( i = 0; i<p_box->data.p_trun->sample_count; i++ )
    {
        mp4_descriptor_trun_sample_t *p_sample = &p_box->data.p_trun->samples[i];
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_DURATION )
            MP4_GET4BYTES( p_sample->duration );
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_SIZE )
            MP4_GET4BYTES( p_sample->size );
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_FLAGS )
            MP4_GET4BYTES( p_sample->flags );
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_TIME_OFFSET )
            MP4_GET4BYTES( p_sample->composition_time_offset );
    }

#ifdef MP4_VERBOSE
     printf( "read box: \"trun\" version %d flags 0x%x sample count %d",
                  p_box->data.p_trun->version,
                  p_box->data.p_trun->flags,
                  p_box->data.p_trun->sample_count );

    for( unsigned int i = 0; i<p_box->data.p_trun->sample_count; i++ )
    {
        mp4_descriptor_trun_sample_t *p_sample = &p_box->data.p_trun->samples[i];
         printf( "read box: \"trun\" sample %4.4d flags 0x%x duration %d size %d composition time offset %d",
                        i, p_sample->i_flags, p_sample->duration,
                        p_sample->i_size, p_sample->composition_time_offset );
    }
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_trunFromBuffer(  stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i;
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_trun_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_trun );

    MP4_GET4BYTES( p_box->data.p_trun->sample_count );

    if( p_box->data.p_trun->flags & MP4_TRUN_DATA_OFFSET )
        MP4_GET4BYTES( p_box->data.p_trun->data_offset );
    if( p_box->data.p_trun->flags & MP4_TRUN_FIRST_FLAGS )
        MP4_GET4BYTES( p_box->data.p_trun->first_sample_flags );

    p_box->data.p_trun->samples =
      calloc( p_box->data.p_trun->sample_count, sizeof(mp4_descriptor_trun_sample_t) );
    if ( p_box->data.p_trun->samples == NULL )
        MP4_READBOX_EXIT( 0 );

    for( i = 0; i<p_box->data.p_trun->sample_count; i++ )
    {
        mp4_descriptor_trun_sample_t *p_sample = &p_box->data.p_trun->samples[i];
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_DURATION )
            MP4_GET4BYTES( p_sample->duration );
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_SIZE )
            MP4_GET4BYTES( p_sample->size );
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_FLAGS )
            MP4_GET4BYTES( p_sample->flags );
        if( p_box->data.p_trun->flags & MP4_TRUN_SAMPLE_TIME_OFFSET )
            MP4_GET4BYTES( p_sample->composition_time_offset );
    }

#ifdef MP4_VERBOSE
     printf( "read box: \"trun\" version %d flags 0x%x sample count %d",
                  p_box->data.p_trun->version,
                  p_box->data.p_trun->flags,
                  p_box->data.p_trun->sample_count );

    for( unsigned int i = 0; i<p_box->data.p_trun->sample_count; i++ )
    {
        mp4_descriptor_trun_sample_t *p_sample = &p_box->data.p_trun->samples[i];
         printf( "read box: \"trun\" sample %4.4d flags 0x%x duration %d size %d composition time offset %d",
                        i, p_sample->i_flags, p_sample->duration,
                        p_sample->i_size, p_sample->composition_time_offset );
    }
#endif

    MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_trun( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_trun->samples );
}


static int MP4_ReadBox_tkhd(  stream_t *p_stream, mp4_box_t *p_box )
{
#ifdef MP4_VERBOSE
    char s_creation_time[128];
    char s_modification_time[128];
    char s_duration[128];
#endif
    double rotation;    //angle in degrees to be rotated clockwise
    double scale[2];    // scale factor; sx = scale[0] , sy = scale[1]
    double translate[2];// amount to translate; tx = translate[0] , ty = translate[1]
    int *matrix = NULL;
    unsigned int i = 0;

    MP4_READBOX_ENTER( mp4_box_data_tkhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_tkhd );

    if( p_box->data.p_tkhd->version )
    {
        MP4_GET8BYTES( p_box->data.p_tkhd->creation_time );
        MP4_GET8BYTES( p_box->data.p_tkhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_tkhd->track_id );
        MP4_GET4BYTES( p_box->data.p_tkhd->reserved );
        MP4_GET8BYTES( p_box->data.p_tkhd->duration );
    }
    else
    {
        MP4_GET4BYTES( p_box->data.p_tkhd->creation_time );
        MP4_GET4BYTES( p_box->data.p_tkhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_tkhd->track_id );
        MP4_GET4BYTES( p_box->data.p_tkhd->reserved );
        MP4_GET4BYTES( p_box->data.p_tkhd->duration );
    }

    for( i = 0; i < 2; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_tkhd->reserved2[i] );
    }
    MP4_GET2BYTES( p_box->data.p_tkhd->layer );
    MP4_GET2BYTES( p_box->data.p_tkhd->predefined );
    MP4_GET2BYTES( p_box->data.p_tkhd->volume );
    MP4_GET2BYTES( p_box->data.p_tkhd->reserved3 );

    for( i = 0; i < 9; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_tkhd->matrix[i] );
    }
    MP4_GET4BYTES( p_box->data.p_tkhd->width );
    MP4_GET4BYTES( p_box->data.p_tkhd->height );

    
    matrix = p_box->data.p_tkhd->matrix;
    
    translate[0] = conv_fx(matrix[6]);
    translate[1] = conv_fx(matrix[7]);
    
    scale[0] = sqrt(conv_fx(matrix[0]) * conv_fx(matrix[0]) +
                    conv_fx(matrix[3]) * conv_fx(matrix[3]));
    scale[1] = sqrt(conv_fx(matrix[1]) * conv_fx(matrix[1]) +
                    conv_fx(matrix[4]) * conv_fx(matrix[4]));
    
    rotation = atan2(conv_fx(matrix[1]) / scale[1], conv_fx(matrix[0]) / scale[0]) * 180 / M_PI;
    
    if (rotation < 0)
        rotation += 360.;

#ifdef MP4_VERBOSE
    MP4_ConvertDate2Str( s_creation_time, p_box->data.p_mvhd->creation_time );
    MP4_ConvertDate2Str( s_modification_time, p_box->data.p_mvhd->modification_time );
    MP4_ConvertDate2Str( s_duration, p_box->data.p_mvhd->duration );

     printf( "read box: \"tkhd\" creation %s modification %s duration %s track ID %d layer %d volume %f rotation %f scaleX %f scaleY %f translateX %f translateY %f width %f height %f. "
            "Matrix: %i %i %i %i %i %i %i %i %i",
                  s_creation_time,
                  s_modification_time,
                  s_duration,
                  p_box->data.p_tkhd->track_ID,
                  p_box->data.p_tkhd->layer,
                  (float)p_box->data.p_tkhd->volume / 256 ,
                  rotation,
                  scale[0],
                  scale[1],
                  translate[0],
                  translate[1],
                  (float)p_box->data.p_tkhd->width / 65536,
                  (float)p_box->data.p_tkhd->height / 65536,
                  p_box->data.p_tkhd->matrix[0],
                  p_box->data.p_tkhd->matrix[1],
                  p_box->data.p_tkhd->matrix[2],
                  p_box->data.p_tkhd->matrix[3],
                  p_box->data.p_tkhd->matrix[4],
                  p_box->data.p_tkhd->matrix[5],
                  p_box->data.p_tkhd->matrix[6],
                  p_box->data.p_tkhd->matrix[7],
                  p_box->data.p_tkhd->matrix[8] );
#endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_tkhdFromBuffer(  stream_t *p_stream, mp4_box_t *p_box )
{
#ifdef MP4_VERBOSE
    char s_creation_time[128];
    char s_modification_time[128];
    char s_duration[128];
#endif
    double rotation;    //angle in degrees to be rotated clockwise
    double scale[2];    // scale factor; sx = scale[0] , sy = scale[1]
    double translate[2];// amount to translate; tx = translate[0] , ty = translate[1]
    int *matrix = NULL;
    unsigned int i = 0;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_tkhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_tkhd );

    if( p_box->data.p_tkhd->version )
    {
        MP4_GET8BYTES( p_box->data.p_tkhd->creation_time );
        MP4_GET8BYTES( p_box->data.p_tkhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_tkhd->track_id );
        MP4_GET4BYTES( p_box->data.p_tkhd->reserved );
        MP4_GET8BYTES( p_box->data.p_tkhd->duration );
    }
    else
    {
        MP4_GET4BYTES( p_box->data.p_tkhd->creation_time );
        MP4_GET4BYTES( p_box->data.p_tkhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_tkhd->track_id );
        MP4_GET4BYTES( p_box->data.p_tkhd->reserved );
        MP4_GET4BYTES( p_box->data.p_tkhd->duration );
    }

    for( i = 0; i < 2; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_tkhd->reserved2[i] );
    }
    MP4_GET2BYTES( p_box->data.p_tkhd->layer );
    MP4_GET2BYTES( p_box->data.p_tkhd->predefined );
    MP4_GET2BYTES( p_box->data.p_tkhd->volume );
    MP4_GET2BYTES( p_box->data.p_tkhd->reserved3 );

    for( i = 0; i < 9; i++ )
    {
        MP4_GET4BYTES( p_box->data.p_tkhd->matrix[i] );
    }
    MP4_GET4BYTES( p_box->data.p_tkhd->width );
    MP4_GET4BYTES( p_box->data.p_tkhd->height );

    
    matrix = p_box->data.p_tkhd->matrix;
    
    translate[0] = conv_fx(matrix[6]);
    translate[1] = conv_fx(matrix[7]);
    
    scale[0] = sqrt(conv_fx(matrix[0]) * conv_fx(matrix[0]) +
                    conv_fx(matrix[3]) * conv_fx(matrix[3]));
    scale[1] = sqrt(conv_fx(matrix[1]) * conv_fx(matrix[1]) +
                    conv_fx(matrix[4]) * conv_fx(matrix[4]));
    
    rotation = atan2(conv_fx(matrix[1]) / scale[1], conv_fx(matrix[0]) / scale[0]) * 180 / M_PI;
    
    if (rotation < 0)
        rotation += 360.;

#ifdef MP4_VERBOSE
    MP4_ConvertDate2Str( s_creation_time, p_box->data.p_mvhd->creation_time );
    MP4_ConvertDate2Str( s_modification_time, p_box->data.p_mvhd->modification_time );
    MP4_ConvertDate2Str( s_duration, p_box->data.p_mvhd->duration );

     printf( "read box: \"tkhd\" creation %s modification %s duration %s track ID %d layer %d volume %f rotation %f scaleX %f scaleY %f translateX %f translateY %f width %f height %f. "
            "Matrix: %i %i %i %i %i %i %i %i %i",
                  s_creation_time,
                  s_modification_time,
                  s_duration,
                  p_box->data.p_tkhd->track_ID,
                  p_box->data.p_tkhd->layer,
                  (float)p_box->data.p_tkhd->volume / 256 ,
                  rotation,
                  scale[0],
                  scale[1],
                  translate[0],
                  translate[1],
                  (float)p_box->data.p_tkhd->width / 65536,
                  (float)p_box->data.p_tkhd->height / 65536,
                  p_box->data.p_tkhd->matrix[0],
                  p_box->data.p_tkhd->matrix[1],
                  p_box->data.p_tkhd->matrix[2],
                  p_box->data.p_tkhd->matrix[3],
                  p_box->data.p_tkhd->matrix[4],
                  p_box->data.p_tkhd->matrix[5],
                  p_box->data.p_tkhd->matrix[6],
                  p_box->data.p_tkhd->matrix[7],
                  p_box->data.p_tkhd->matrix[8] );
#endif

    MP4_READBOX_EXIT( 1 );
}



static int MP4_ReadBox_hint( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_hint_t );

   MP4_GET4BYTES( p_box->data.p_hint->track_IDs );

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_hintFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_hint_t );

   MP4_GET4BYTES( p_box->data.p_hint->track_IDs );

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mdhd( stream_t *p_stream, mp4_box_t *p_box )
{
    uint16_t i_language;
    unsigned int i = 0;
#ifdef MP4_VERBOSE
    char s_creation_time[128];
    char s_modification_time[128];
    char s_duration[128];
#endif
    MP4_READBOX_ENTER( mp4_box_data_mdhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_mdhd );

    if( p_box->data.p_mdhd->version )
    {
        MP4_GET8BYTES( p_box->data.p_mdhd->creation_time );
        MP4_GET8BYTES( p_box->data.p_mdhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mdhd->timescale );
        MP4_GET8BYTES( p_box->data.p_mdhd->duration );
    }
    else
    {
        MP4_GET4BYTES( p_box->data.p_mdhd->creation_time );
        MP4_GET4BYTES( p_box->data.p_mdhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mdhd->timescale );
        MP4_GET4BYTES( p_box->data.p_mdhd->duration );
    }
    p_box->data.p_mdhd->language_code = i_language = SwapBE16( p_peek );
    for( i = 0; i < 3; i++ )
    {
        p_box->data.p_mdhd->language[i] =
                    ( ( i_language >> ( (2-i)*5 ) )&0x1f ) + 0x60;
    }

    MP4_GET2BYTES( p_box->data.p_mdhd->predefined );

// #ifdef MP4_VERBOSE
//     MP4_ConvertDate2Str( s_creation_time, p_box->data.p_mdhd->creation_time );
//     MP4_ConvertDate2Str( s_modification_time, p_box->data.p_mdhd->modification_time );
//     MP4_ConvertDate2Str( s_duration, p_box->data.p_mdhd->duration );
//      printf( "read box: \"mdhd\" creation %s modification %s time scale %d duration %s language %c%c%c",
//                   s_creation_time,
//                   s_modification_time,
//                   (uint32_t)p_box->data.p_mdhd->timescale,
//                   s_duration,
//                   p_box->data.p_mdhd->language[0],
//                   p_box->data.p_mdhd->language[1],
//                   p_box->data.p_mdhd->language[2] );
// #endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_mdhdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    uint16_t i_language;
    unsigned int i = 0;
#ifdef MP4_VERBOSE
    char s_creation_time[128];
    char s_modification_time[128];
    char s_duration[128];
#endif
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_mdhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_mdhd );

    if( p_box->data.p_mdhd->version )
    {
        MP4_GET8BYTES( p_box->data.p_mdhd->creation_time );
        MP4_GET8BYTES( p_box->data.p_mdhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mdhd->timescale );
        MP4_GET8BYTES( p_box->data.p_mdhd->duration );
    }
    else
    {
        MP4_GET4BYTES( p_box->data.p_mdhd->creation_time );
        MP4_GET4BYTES( p_box->data.p_mdhd->modification_time );
        MP4_GET4BYTES( p_box->data.p_mdhd->timescale );
        MP4_GET4BYTES( p_box->data.p_mdhd->duration );
    }
    p_box->data.p_mdhd->language_code = i_language = SwapBE16( p_peek );
    for( i = 0; i < 3; i++ )
    {
        p_box->data.p_mdhd->language[i] =
                    ( ( i_language >> ( (2-i)*5 ) )&0x1f ) + 0x60;
    }

    MP4_GET2BYTES( p_box->data.p_mdhd->predefined );

// #ifdef MP4_VERBOSE
//     MP4_ConvertDate2Str( s_creation_time, p_box->data.p_mdhd->creation_time );
//     MP4_ConvertDate2Str( s_modification_time, p_box->data.p_mdhd->modification_time );
//     MP4_ConvertDate2Str( s_duration, p_box->data.p_mdhd->duration );
//      printf( "read box: \"mdhd\" creation %s modification %s time scale %d duration %s language %c%c%c",
//                   s_creation_time,
//                   s_modification_time,
//                   (uint32_t)p_box->data.p_mdhd->timescale,
//                   s_duration,
//                   p_box->data.p_mdhd->language[0],
//                   p_box->data.p_mdhd->language[1],
//                   p_box->data.p_mdhd->language[2] );
// #endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_hdlr( stream_t *p_stream, mp4_box_t *p_box )
{
    int32_t i_reserved;

    MP4_READBOX_ENTER( mp4_box_data_hdlr_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_hdlr );

    MP4_GETFOURCC( p_box->data.p_hdlr->predefined );
    MP4_GETFOURCC( p_box->data.p_hdlr->handler_type );

    MP4_GET4BYTES( i_reserved );
    MP4_GET4BYTES( i_reserved );
    MP4_GET4BYTES( i_reserved );
    p_box->data.p_hdlr->psz_name = NULL;

    if( i_read > 0 )
    {
        uint8_t *psz = p_box->data.p_hdlr->psz_name = malloc( i_read + 1 );
        if( unlikely( psz == NULL ) )
            MP4_READBOX_EXIT( 0 );

        /* Yes, I love .mp4 :( */
        if( p_box->data.p_hdlr->predefined == MP4_FOURCC( 'm', 'h', 'l', 'r' ) )
        {
            uint8_t i_len;
            int i_copy;

            MP4_GET1BYTE( i_len );
            i_copy = min( i_read, i_len );

            memcpy( psz, p_peek, i_copy );
            p_box->data.p_hdlr->psz_name[i_copy] = '\0';
        }
        else
        {
            memcpy( psz, p_peek, i_read );
            p_box->data.p_hdlr->psz_name[i_read] = '\0';
        }
    }

#ifdef MP4_VERBOSE
         printf( "read box: \"hdlr\" handler type: \"%4.4s\" name: \"%s\"",
                   (char*)&p_box->data.p_hdlr->handler_type,
                   p_box->data.p_hdlr->psz_name );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_hdlrFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    int32_t i_reserved;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_hdlr_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_hdlr );

    MP4_GETFOURCC( p_box->data.p_hdlr->predefined );
    MP4_GETFOURCC( p_box->data.p_hdlr->handler_type );

    MP4_GET4BYTES( i_reserved );
    MP4_GET4BYTES( i_reserved );
    MP4_GET4BYTES( i_reserved );
    p_box->data.p_hdlr->psz_name = NULL;

    if( i_read > 0 )
    {
        uint8_t *psz = p_box->data.p_hdlr->psz_name = malloc( i_read + 1 );
        if( unlikely( psz == NULL ) )
            MP4_READBOX_EXIT( 0 );

        /* Yes, I love .mp4 :( */
        if( p_box->data.p_hdlr->predefined == MP4_FOURCC( 'm', 'h', 'l', 'r' ) )
        {
            uint8_t i_len;
            int i_copy;

            MP4_GET1BYTE( i_len );
            i_copy = min( i_read, i_len );

            memcpy( psz, p_peek, i_copy );
            p_box->data.p_hdlr->psz_name[i_copy] = '\0';
        }
        else
        {
            memcpy( psz, p_peek, i_read );
            p_box->data.p_hdlr->psz_name[i_read] = '\0';
        }
    }

#ifdef MP4_VERBOSE
         printf( "read box: \"hdlr\" handler type: \"%4.4s\" name: \"%s\"",
                   (char*)&p_box->data.p_hdlr->handler_type,
                   p_box->data.p_hdlr->psz_name );
#endif

    MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_hdlr( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_hdlr->psz_name );
}

static int MP4_ReadBox_vmhd( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_vmhd_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_vmhd );

   MP4_GET2BYTES( p_box->data.p_vmhd->graphics_mode );
   for( i = 0; i < 3; i++ )
   {
      MP4_GET2BYTES( p_box->data.p_vmhd->opcolor[i] );
   }

#ifdef MP4_VERBOSE
     printf( "read box: \"vmhd\" graphics-mode %d opcolor (%d, %d, %d)",
                      p_box->data.p_vmhd->i_graphics_mode,
                      p_box->data.p_vmhd->i_opcolor[0],
                      p_box->data.p_vmhd->i_opcolor[1],
                      p_box->data.p_vmhd->i_opcolor[2] );
#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_vmhdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_vmhd_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_vmhd );

   MP4_GET2BYTES( p_box->data.p_vmhd->graphics_mode );
   for( i = 0; i < 3; i++ )
   {
      MP4_GET2BYTES( p_box->data.p_vmhd->opcolor[i] );
   }

#ifdef MP4_VERBOSE
     printf( "read box: \"vmhd\" graphics-mode %d opcolor (%d, %d, %d)",
                      p_box->data.p_vmhd->i_graphics_mode,
                      p_box->data.p_vmhd->i_opcolor[0],
                      p_box->data.p_vmhd->i_opcolor[1],
                      p_box->data.p_vmhd->i_opcolor[2] );
#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_smhd( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_smhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_smhd );



    MP4_GET2BYTES( p_box->data.p_smhd->balance );

    MP4_GET2BYTES( p_box->data.p_smhd->reserved );

// #ifdef MP4_VERBOSE
//      printf( "read box: \"smhd\" balance %f",
//                       (float)p_box->data.p_smhd->balance / 256 );
// #endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_smhdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_smhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_smhd );



    MP4_GET2BYTES( p_box->data.p_smhd->balance );

    MP4_GET2BYTES( p_box->data.p_smhd->reserved );

// #ifdef MP4_VERBOSE
//      printf( "read box: \"smhd\" balance %f",
//                       (float)p_box->data.p_smhd->balance / 256 );
// #endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_hmhd( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_hmhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_hmhd );

    MP4_GET2BYTES( p_box->data.p_hmhd->max_PDU_size );
    MP4_GET2BYTES( p_box->data.p_hmhd->avg_PDU_size );

    MP4_GET4BYTES( p_box->data.p_hmhd->max_bitrate );
    MP4_GET4BYTES( p_box->data.p_hmhd->avg_bitrate );

    MP4_GET4BYTES( p_box->data.p_hmhd->reserved );

#ifdef MP4_VERBOSE
     printf( "read box: \"hmhd\" maxPDU-size %d avgPDU-size %d max-bitrate %d avg-bitrate %d",
                      p_box->data.p_hmhd->max_PDU_size,
                      p_box->data.p_hmhd->avg_PDU_size,
                      p_box->data.p_hmhd->max_bitrate,
                      p_box->data.p_hmhd->avg_bitrate );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_hmhdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_hmhd_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_hmhd );

    MP4_GET2BYTES( p_box->data.p_hmhd->max_PDU_size );
    MP4_GET2BYTES( p_box->data.p_hmhd->avg_PDU_size );

    MP4_GET4BYTES( p_box->data.p_hmhd->max_bitrate );
    MP4_GET4BYTES( p_box->data.p_hmhd->avg_bitrate );

    MP4_GET4BYTES( p_box->data.p_hmhd->reserved );

#ifdef MP4_VERBOSE
     printf( "read box: \"hmhd\" maxPDU-size %d avgPDU-size %d max-bitrate %d avg-bitrate %d",
                      p_box->data.p_hmhd->max_PDU_size,
                      p_box->data.p_hmhd->avg_PDU_size,
                      p_box->data.p_hmhd->max_bitrate,
                      p_box->data.p_hmhd->avg_bitrate );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_url( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_url_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_url );
    MP4_GETSTRINGZ( p_box->data.p_url->psz_location );

#ifdef MP4_VERBOSE
     printf( "read box: \"url\" url: %s",
                       p_box->data.p_url->psz_location );

#endif
    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_urlFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_url_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_url );
    MP4_GETSTRINGZ( p_box->data.p_url->psz_location );

#ifdef MP4_VERBOSE
     printf( "read box: \"url\" url: %s",
                       p_box->data.p_url->psz_location );

#endif
    MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_url( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_url->psz_location );
}

static int MP4_ReadBox_urn( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_urn_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_urn );

    MP4_GETSTRINGZ( p_box->data.p_urn->psz_name );
    MP4_GETSTRINGZ( p_box->data.p_urn->psz_location );

#ifdef MP4_VERBOSE
     printf( "read box: \"urn\" name %s location %s",
                      p_box->data.p_urn->psz_name,
                      p_box->data.p_urn->psz_location );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_urnFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_urn_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_urn );

    MP4_GETSTRINGZ( p_box->data.p_urn->psz_name );
    MP4_GETSTRINGZ( p_box->data.p_urn->psz_location );

#ifdef MP4_VERBOSE
     printf( "read box: \"urn\" name %s location %s",
                      p_box->data.p_urn->psz_name,
                      p_box->data.p_urn->psz_location );
#endif

    MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_urn( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_urn->psz_name );
    FREENULL( p_box->data.p_urn->psz_location );
}


static int MP4_ReadBox_dref( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTER( mp4_box_data_dref_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_dref );

    MP4_GET4BYTES( p_box->data.p_dref->entry_count );

    stream_seek( p_stream, p_box->i_pos + mp4_box_headersize( p_box ) + 8, SEEK_SET );
    MP4_ReadBoxContainerRaw( p_stream, p_box );

#ifdef MP4_VERBOSE
     printf( "read box: \"dref\" entry-count %d",
                      p_box->data.p_dref->entry_count );
#endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_drefFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_dref_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_dref );

    MP4_GET4BYTES( p_box->data.p_dref->entry_count );

    buffer_seek( p_stream, mp4_box_headersize( p_box ) + 8, p_box->i_pos );
    MP4_ReadBoxContainerRawFromBuffer( p_stream, p_box );

#ifdef MP4_VERBOSE
     printf( "read box: \"dref\" entry-count %d",
                      p_box->data.p_dref->entry_count );
#endif

    MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_stts( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_stts->sample_count );
    FREENULL( p_box->data.p_stts->sample_delta );
}

static int MP4_ReadBox_stts( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i;
   MP4_READBOX_ENTER( mp4_box_data_stts_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stts );
   MP4_GET4BYTES( p_box->data.p_stts->entry_count );

   p_box->data.p_stts->sample_count =
      calloc( p_box->data.p_stts->entry_count, sizeof(uint32_t) );
   p_box->data.p_stts->sample_delta =
      calloc( p_box->data.p_stts->entry_count, sizeof(uint32_t) );
   if( p_box->data.p_stts->sample_count == NULL
      || p_box->data.p_stts->sample_delta == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_stts->entry_count )&&( i_read >=8 ); i++ )
   {
      MP4_GET4BYTES( p_box->data.p_stts->sample_count[i] );
      MP4_GET4BYTES( p_box->data.p_stts->sample_delta[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stts\" entry-count %d",
      p_box->data.p_stts->entry_count );

#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_sttsFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_stts_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stts );
   MP4_GET4BYTES( p_box->data.p_stts->entry_count );

   p_box->data.p_stts->sample_count =
      calloc( p_box->data.p_stts->entry_count, sizeof(uint32_t) );
   p_box->data.p_stts->sample_delta =
      calloc( p_box->data.p_stts->entry_count, sizeof(uint32_t) );
   if( p_box->data.p_stts->sample_count == NULL
      || p_box->data.p_stts->sample_delta == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_stts->entry_count )&&( i_read >=8 ); i++ )
   {
      MP4_GET4BYTES( p_box->data.p_stts->sample_count[i] );
      MP4_GET4BYTES( p_box->data.p_stts->sample_delta[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stts\" entry-count %d",
      p_box->data.p_stts->entry_count );

#endif

   MP4_READBOX_EXIT( 1 );
}


static void MP4_FreeBox_ctts( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_ctts->sample_count );
    FREENULL( p_box->data.p_ctts->sample_offset );
}

static int MP4_ReadBox_ctts( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_ctts_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_ctts );

   MP4_GET4BYTES( p_box->data.p_ctts->entry_count );

   p_box->data.p_ctts->sample_count =
      calloc( p_box->data.p_ctts->entry_count, sizeof(uint32_t) );
   p_box->data.p_ctts->sample_offset =
      calloc( p_box->data.p_ctts->entry_count, sizeof(uint32_t) );
   if( ( p_box->data.p_ctts->sample_count == NULL )
      || ( p_box->data.p_ctts->sample_offset == NULL ) )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_ctts->entry_count )&&( i_read >=8 ); i++ )
   {
      MP4_GET4BYTES( p_box->data.p_ctts->sample_count[i] );
      MP4_GET4BYTES( p_box->data.p_ctts->sample_offset[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"ctts\" entry-count %d",
      p_box->data.p_ctts->entry_count );

#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_cttsFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_ctts_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_ctts );

   MP4_GET4BYTES( p_box->data.p_ctts->entry_count );

   p_box->data.p_ctts->sample_count =
      calloc( p_box->data.p_ctts->entry_count, sizeof(uint32_t) );
   p_box->data.p_ctts->sample_offset =
      calloc( p_box->data.p_ctts->entry_count, sizeof(uint32_t) );
   if( ( p_box->data.p_ctts->sample_count == NULL )
      || ( p_box->data.p_ctts->sample_offset == NULL ) )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_ctts->entry_count )&&( i_read >=8 ); i++ )
   {
      MP4_GET4BYTES( p_box->data.p_ctts->sample_count[i] );
      MP4_GET4BYTES( p_box->data.p_ctts->sample_offset[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"ctts\" entry-count %d",
      p_box->data.p_ctts->entry_count );

#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadLengthDescriptor( uint8_t **pp_peek, int64_t  *i_read )
{
    unsigned int i_b;
    unsigned int i_len = 0;
    do
    {
        i_b = **pp_peek;

        (*pp_peek)++;
        (*i_read)--;
        i_len = ( i_len << 7 ) + ( i_b&0x7f );
    } while( i_b&0x80 );
    return( i_len );
}


static void MP4_FreeBox_esds( mp4_box_t *p_box )
{
    FREENULL( p_box->data.p_esds->es_descriptor.psz_URL );
    if( p_box->data.p_esds->es_descriptor.decConfigDescr )
    {
        FREENULL( p_box->data.p_esds->es_descriptor.decConfigDescr->decoder_specific_info );
        FREENULL( p_box->data.p_esds->es_descriptor.decConfigDescr );
    }
}

static int MP4_ReadBox_esds( stream_t *p_stream, mp4_box_t *p_box )
{
#define es_descriptor p_box->data.p_esds->es_descriptor
    unsigned int i_len;
    unsigned int i_flags;
    unsigned int i_type;

    MP4_READBOX_ENTER( mp4_box_data_esds_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_esds );


    MP4_GET1BYTE( i_type );
    if( i_type == 0x03 ) /* MP4ESDescrTag */
    {
        i_len = MP4_ReadLengthDescriptor( &p_peek, &i_read );

#ifdef MP4_VERBOSE
         printf( "found esds MPEG4ESDescr (%dBytes)",
                 i_len );
#endif

        MP4_GET2BYTES( es_descriptor.ES_ID );
        MP4_GET1BYTE( i_flags );
        es_descriptor.b_stream_dependence = ( (i_flags&0x80) != 0);
        es_descriptor.b_url = ( (i_flags&0x40) != 0);
        es_descriptor.b_OCRstream = ( (i_flags&0x20) != 0);

        es_descriptor.stream_priority = i_flags&0x1f;
        if( es_descriptor.b_stream_dependence )
        {
            MP4_GET2BYTES( es_descriptor.depend_on_ES_ID );
        }
        if( es_descriptor.b_url )
        {
            unsigned int i_len;

            MP4_GET1BYTE( i_len );
            es_descriptor.psz_URL = malloc( i_len + 1 );
            if( es_descriptor.psz_URL )
            {
                memcpy( es_descriptor.psz_URL, p_peek, i_len );
                es_descriptor.psz_URL[i_len] = 0;
            }
            p_peek += i_len;
            i_read -= i_len;
        }
        else
        {
            es_descriptor.psz_URL = NULL;
        }
        if( es_descriptor.b_OCRstream )
        {
            MP4_GET2BYTES( es_descriptor.OCR_ES_ID );
        }
        MP4_GET1BYTE( i_type ); /* get next type */
    }

    if( i_type != 0x04)/* MP4DecConfigDescrTag */
    {
         es_descriptor.decConfigDescr = NULL;
         MP4_READBOX_EXIT( 1 ); /* rest isn't interesting up to now */
    }

    i_len = MP4_ReadLengthDescriptor( &p_peek, &i_read );

#ifdef MP4_VERBOSE
         printf( "found esds MP4DecConfigDescr (%dBytes)",
                 i_len );
#endif

    es_descriptor.decConfigDescr =
            calloc( 1, sizeof( mp4_descriptor_decoder_config_t ));
    if( unlikely( es_descriptor.decConfigDescr == NULL ) )
        MP4_READBOX_EXIT( 0 );

    MP4_GET1BYTE( es_descriptor.decConfigDescr->objectTypeIndication );
    MP4_GET1BYTE( i_flags );
    es_descriptor.decConfigDescr->streamType = i_flags >> 2;
    es_descriptor.decConfigDescr->b_upStream = ( i_flags >> 1 )&0x01;
    MP4_GET3BYTES( es_descriptor.decConfigDescr->buffer_sizeDB );
    MP4_GET4BYTES( es_descriptor.decConfigDescr->max_bitrate );
    MP4_GET4BYTES( es_descriptor.decConfigDescr->avg_bitrate );
    MP4_GET1BYTE( i_type );
    if( i_type !=  0x05 )/* MP4DecSpecificDescrTag */
    {
        es_descriptor.decConfigDescr->decoder_specific_info_len = 0;
        es_descriptor.decConfigDescr->decoder_specific_info  = NULL;
        MP4_READBOX_EXIT( 1 );
    }

    i_len = MP4_ReadLengthDescriptor( &p_peek, &i_read );

#ifdef MP4_VERBOSE
         printf( "found esds MP4DecSpecificDescr (%dBytes)",
                 i_len );
#endif
    if( i_len > i_read )
        MP4_READBOX_EXIT( 0 );

    es_descriptor.decConfigDescr->decoder_specific_info_len = i_len;
    es_descriptor.decConfigDescr->decoder_specific_info = malloc( i_len );
    if( unlikely( es_descriptor.decConfigDescr->decoder_specific_info == NULL ) )
        MP4_READBOX_EXIT( 0 );

    memcpy( es_descriptor.decConfigDescr->decoder_specific_info,
            p_peek, i_len );

    MP4_READBOX_EXIT( 1 );
#undef es_descriptor
}

static int MP4_ReadBox_esdsFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
#define es_descriptor p_box->data.p_esds->es_descriptor
    unsigned int i_len;
    unsigned int i_flags;
    unsigned int i_type;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_esds_t );

    MP4_GETVERSIONFLAGS( p_box->data.p_esds );


    MP4_GET1BYTE( i_type );
    if( i_type == 0x03 ) /* MP4ESDescrTag */
    {
        i_len = MP4_ReadLengthDescriptor( &p_peek, &i_read );

#ifdef MP4_VERBOSE
         printf( "found esds MPEG4ESDescr (%dBytes)",
                 i_len );
#endif

        MP4_GET2BYTES( es_descriptor.ES_ID );
        MP4_GET1BYTE( i_flags );
        es_descriptor.b_stream_dependence = ( (i_flags&0x80) != 0);
        es_descriptor.b_url = ( (i_flags&0x40) != 0);
        es_descriptor.b_OCRstream = ( (i_flags&0x20) != 0);

        es_descriptor.stream_priority = i_flags&0x1f;
        if( es_descriptor.b_stream_dependence )
        {
            MP4_GET2BYTES( es_descriptor.depend_on_ES_ID );
        }
        if( es_descriptor.b_url )
        {
            unsigned int i_len;

            MP4_GET1BYTE( i_len );
            es_descriptor.psz_URL = malloc( i_len + 1 );
            if( es_descriptor.psz_URL )
            {
                memcpy( es_descriptor.psz_URL, p_peek, i_len );
                es_descriptor.psz_URL[i_len] = 0;
            }
            p_peek += i_len;
            i_read -= i_len;
        }
        else
        {
            es_descriptor.psz_URL = NULL;
        }
        if( es_descriptor.b_OCRstream )
        {
            MP4_GET2BYTES( es_descriptor.OCR_ES_ID );
        }
        MP4_GET1BYTE( i_type ); /* get next type */
    }

    if( i_type != 0x04)/* MP4DecConfigDescrTag */
    {
         es_descriptor.decConfigDescr = NULL;
         MP4_READBOX_EXIT( 1 ); /* rest isn't interesting up to now */
    }

    i_len = MP4_ReadLengthDescriptor( &p_peek, &i_read );

#ifdef MP4_VERBOSE
         printf( "found esds MP4DecConfigDescr (%dBytes)",
                 i_len );
#endif

    es_descriptor.decConfigDescr =
            calloc( 1, sizeof( mp4_descriptor_decoder_config_t ));
    if( unlikely( es_descriptor.decConfigDescr == NULL ) )
        MP4_READBOX_EXIT( 0 );

    MP4_GET1BYTE( es_descriptor.decConfigDescr->objectTypeIndication );
    MP4_GET1BYTE( i_flags );
    es_descriptor.decConfigDescr->streamType = i_flags >> 2;
    es_descriptor.decConfigDescr->b_upStream = ( i_flags >> 1 )&0x01;
    MP4_GET3BYTES( es_descriptor.decConfigDescr->buffer_sizeDB );
    MP4_GET4BYTES( es_descriptor.decConfigDescr->max_bitrate );
    MP4_GET4BYTES( es_descriptor.decConfigDescr->avg_bitrate );
    MP4_GET1BYTE( i_type );
    if( i_type !=  0x05 )/* MP4DecSpecificDescrTag */
    {
        es_descriptor.decConfigDescr->decoder_specific_info_len = 0;
        es_descriptor.decConfigDescr->decoder_specific_info  = NULL;
        MP4_READBOX_EXIT( 1 );
    }

    i_len = MP4_ReadLengthDescriptor( &p_peek, &i_read );

#ifdef MP4_VERBOSE
         printf( "found esds MP4DecSpecificDescr (%dBytes)",
                 i_len );
#endif
    if( i_len > i_read )
        MP4_READBOX_EXIT( 0 );

    es_descriptor.decConfigDescr->decoder_specific_info_len = i_len;
    es_descriptor.decConfigDescr->decoder_specific_info = malloc( i_len );
    if( unlikely( es_descriptor.decConfigDescr->decoder_specific_info == NULL ) )
        MP4_READBOX_EXIT( 0 );

    memcpy( es_descriptor.decConfigDescr->decoder_specific_info,
            p_peek, i_len );

    MP4_READBOX_EXIT( 1 );
#undef es_descriptor
}

static void MP4_FreeBox_avcC( mp4_box_t *p_box )
{
    mp4_box_data_avcC_t *p_avcC = p_box->data.p_avcC;
    int i;

    if( p_avcC->avcC > 0 ) FREENULL( p_avcC->p_avcC );

    if( p_avcC->p_sps )
    {
        for( i = 0; i < p_avcC->sps; i++ )
            FREENULL( p_avcC->p_sps[i] );
    }
    if( p_avcC->p_pps )
    {
        for( i = 0; i < p_avcC->pps; i++ )
            FREENULL( p_avcC->p_pps[i] );
    }
    if( p_avcC->sps > 0 ) FREENULL( p_avcC->p_sps );
    if( p_avcC->sps > 0 ) FREENULL( p_avcC->sps_length );
    if( p_avcC->pps > 0 ) FREENULL( p_avcC->p_pps );
    if( p_avcC->pps > 0 ) FREENULL( p_avcC->pps_length );
}

static int MP4_ReadBox_avcC( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_avcC_t *p_avcC;
    int i;

    MP4_READBOX_ENTER( mp4_box_data_avcC_t );
    p_avcC = p_box->data.p_avcC;

    p_avcC->avcC = i_read;
    if( p_avcC->avcC > 0 )
    {
        uint8_t * p = p_avcC->p_avcC = malloc( p_avcC->avcC );
        if( p )
            memcpy( p, p_peek, i_read );
    }

    MP4_GET1BYTE( p_avcC->version );
    MP4_GET1BYTE( p_avcC->profile );
    MP4_GET1BYTE( p_avcC->profile_compatibility );
    MP4_GET1BYTE( p_avcC->level );
    MP4_GET1BYTE( p_avcC->reserved1 );
    p_avcC->length_size = (p_avcC->reserved1&0x03) + 1;
    p_avcC->reserved1 >>= 2;

    MP4_GET1BYTE( p_avcC->reserved2 );
    p_avcC->sps = p_avcC->reserved2&0x1f;
    p_avcC->reserved2 >>= 5;

    if( p_avcC->sps > 0 )
    {
        p_avcC->sps_length = calloc( p_avcC->sps, sizeof( uint16_t ) );
        p_avcC->p_sps = calloc( p_avcC->sps, sizeof( uint8_t* ) );

        if( !p_avcC->sps_length || !p_avcC->p_sps )
            goto error;

        for( i = 0; i < p_avcC->sps; i++ )
        {
            MP4_GET2BYTES( p_avcC->sps_length[i] );
            p_avcC->p_sps[i] = malloc( p_avcC->sps_length[i] );
            if( p_avcC->p_sps[i] )
                memcpy( p_avcC->p_sps[i], p_peek, p_avcC->sps_length[i] );

            p_peek += p_avcC->sps_length[i];
            i_read -= p_avcC->sps_length[i];
        }
    }

    MP4_GET1BYTE( p_avcC->pps );
    if( p_avcC->pps > 0 )
    {
        p_avcC->pps_length = calloc( p_avcC->pps, sizeof( uint16_t ) );
        p_avcC->p_pps = calloc( p_avcC->pps, sizeof( uint8_t* ) );

        if( !p_avcC->pps_length || !p_avcC->p_pps )
            goto error;

        for( i = 0; i < p_avcC->pps; i++ )
        {
            MP4_GET2BYTES( p_avcC->pps_length[i] );
            p_avcC->p_pps[i] = malloc( p_avcC->pps_length[i] );
            if( p_avcC->p_pps[i] )
                memcpy( p_avcC->p_pps[i], p_peek, p_avcC->pps_length[i] );

            p_peek += p_avcC->pps_length[i];
            i_read -= p_avcC->pps_length[i];
        }
    }
// #ifdef MP4_VERBOSE
//      printf(
//              "read box: \"avcC\" version=%d profile=0x%x level=0x%x length size=%d sps=%d pps=%d",
//              p_avcC->i_version, p_avcC->i_profile, p_avcC->i_level,
//              p_avcC->i_length_size,
//              p_avcC->i_sps, p_avcC->i_pps );
//     for( i = 0; i < p_avcC->i_sps; i++ )
//     {
//          printf( "         - sps[%d] length=%d",
//                  i, p_avcC->i_sps_length[i] );
//     }
//     for( i = 0; i < p_avcC->i_pps; i++ )
//     {
//          printf( "         - pps[%d] length=%d",
//                  i, p_avcC->i_pps_length[i] );
//     }
// #endif

    MP4_READBOX_EXIT( 1 );

error:
    MP4_READBOX_EXIT( 0 );
}

static int MP4_ReadBox_avcCFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_avcC_t *p_avcC;
    int i;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_avcC_t );
    p_avcC = p_box->data.p_avcC;

    p_avcC->avcC = i_read;
    if( p_avcC->avcC > 0 )
    {
        uint8_t * p = p_avcC->p_avcC = malloc( p_avcC->avcC );
        if( p )
            memcpy( p, p_peek, i_read );
    }

    MP4_GET1BYTE( p_avcC->version );
    MP4_GET1BYTE( p_avcC->profile );
    MP4_GET1BYTE( p_avcC->profile_compatibility );
    MP4_GET1BYTE( p_avcC->level );
    MP4_GET1BYTE( p_avcC->reserved1 );
    p_avcC->length_size = (p_avcC->reserved1&0x03) + 1;
    p_avcC->reserved1 >>= 2;

    MP4_GET1BYTE( p_avcC->reserved2 );
    p_avcC->sps = p_avcC->reserved2&0x1f;
    p_avcC->reserved2 >>= 5;

    if( p_avcC->sps > 0 )
    {
        p_avcC->sps_length = calloc( p_avcC->sps, sizeof( uint16_t ) );
        p_avcC->p_sps = calloc( p_avcC->sps, sizeof( uint8_t* ) );

        if( !p_avcC->sps_length || !p_avcC->p_sps )
            goto error;

        for( i = 0; i < p_avcC->sps; i++ )
        {
            MP4_GET2BYTES( p_avcC->sps_length[i] );
            p_avcC->p_sps[i] = malloc( p_avcC->sps_length[i] );
            if( p_avcC->p_sps[i] )
                memcpy( p_avcC->p_sps[i], p_peek, p_avcC->sps_length[i] );

            p_peek += p_avcC->sps_length[i];
            i_read -= p_avcC->sps_length[i];
        }
    }

    MP4_GET1BYTE( p_avcC->pps );
    if( p_avcC->pps > 0 )
    {
        p_avcC->pps_length = calloc( p_avcC->pps, sizeof( uint16_t ) );
        p_avcC->p_pps = calloc( p_avcC->pps, sizeof( uint8_t* ) );

        if( !p_avcC->pps_length || !p_avcC->p_pps )
            goto error;

        for( i = 0; i < p_avcC->pps; i++ )
        {
            MP4_GET2BYTES( p_avcC->pps_length[i] );
            p_avcC->p_pps[i] = malloc( p_avcC->pps_length[i] );
            if( p_avcC->p_pps[i] )
                memcpy( p_avcC->p_pps[i], p_peek, p_avcC->pps_length[i] );

            p_peek += p_avcC->pps_length[i];
            i_read -= p_avcC->pps_length[i];
        }
    }
// #ifdef MP4_VERBOSE
//      printf(
//              "read box: \"avcC\" version=%d profile=0x%x level=0x%x length size=%d sps=%d pps=%d",
//              p_avcC->i_version, p_avcC->i_profile, p_avcC->i_level,
//              p_avcC->i_length_size,
//              p_avcC->i_sps, p_avcC->i_pps );
//     for( i = 0; i < p_avcC->i_sps; i++ )
//     {
//          printf( "         - sps[%d] length=%d",
//                  i, p_avcC->i_sps_length[i] );
//     }
//     for( i = 0; i < p_avcC->i_pps; i++ )
//     {
//          printf( "         - pps[%d] length=%d",
//                  i, p_avcC->i_pps_length[i] );
//     }
// #endif

    MP4_READBOX_EXIT( 1 );

error:
    MP4_READBOX_EXIT( 0 );
}

static int MP4_ReadBox_dac3( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_dac3_t *p_dac3;
    unsigned i_header;

    MP4_READBOX_ENTER( mp4_box_data_dac3_t );

    p_dac3 = p_box->data.p_dac3;
    
    MP4_GET3BYTES( i_header );

    p_dac3->fscod = ( i_header >> 22 ) & 0x03;
    p_dac3->bsid  = ( i_header >> 17 ) & 0x01f;
    p_dac3->bsmod = ( i_header >> 14 ) & 0x07;
    p_dac3->acmod = ( i_header >> 11 ) & 0x07;
    p_dac3->lfeon = ( i_header >> 10 ) & 0x01;
    p_dac3->bitrate_code = ( i_header >> 5) & 0x1f;

#ifdef MP4_VERBOSE
     printf(
             "read box: \"dac3\" fscod=0x%x bsid=0x%x bsmod=0x%x acmod=0x%x lfeon=0x%x bitrate_code=0x%x",
             p_dac3->i_fscod, p_dac3->i_bsid, p_dac3->i_bsmod, p_dac3->i_acmod, p_dac3->i_lfeon, p_dac3->i_bitrate_code );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_dac3FromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_dac3_t *p_dac3;
    unsigned i_header;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_dac3_t );

    p_dac3 = p_box->data.p_dac3;
    
    MP4_GET3BYTES( i_header );

    p_dac3->fscod = ( i_header >> 22 ) & 0x03;
    p_dac3->bsid  = ( i_header >> 17 ) & 0x01f;
    p_dac3->bsmod = ( i_header >> 14 ) & 0x07;
    p_dac3->acmod = ( i_header >> 11 ) & 0x07;
    p_dac3->lfeon = ( i_header >> 10 ) & 0x01;
    p_dac3->bitrate_code = ( i_header >> 5) & 0x1f;

#ifdef MP4_VERBOSE
     printf(
             "read box: \"dac3\" fscod=0x%x bsid=0x%x bsmod=0x%x acmod=0x%x lfeon=0x%x bitrate_code=0x%x",
             p_dac3->i_fscod, p_dac3->i_bsid, p_dac3->i_bsmod, p_dac3->i_acmod, p_dac3->i_lfeon, p_dac3->i_bitrate_code );
#endif

    MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_enda( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_enda_t *p_enda;
    MP4_READBOX_ENTER( mp4_box_data_enda_t );

    p_enda = p_box->data.p_enda;

    MP4_GET2BYTES( p_enda->little_endian );

#ifdef MP4_VERBOSE
     printf(
             "read box: \"enda\" little_endian=%d", p_enda->i_little_endian );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_endaFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_enda_t *p_enda;
    MP4_READBOX_ENTERFromBuffer( mp4_box_data_enda_t );

    p_enda = p_box->data.p_enda;

    MP4_GET2BYTES( p_enda->little_endian );

#ifdef MP4_VERBOSE
     printf(
             "read box: \"enda\" little_endian=%d", p_enda->i_little_endian );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_gnre( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_gnre_t *p_gnre;
    uint32_t i_data_len;
    uint32_t i_data_tag;
    uint32_t i_version;
    uint32_t i_reserved;

    MP4_READBOX_ENTER( mp4_box_data_gnre_t );

    p_gnre = p_box->data.p_gnre;

    MP4_GET4BYTES( i_data_len );
    MP4_GETFOURCC( i_data_tag );
    if( i_data_len < 10 || i_data_tag != ATOM_data )
        MP4_READBOX_EXIT( 0 );

    MP4_GET4BYTES( i_version );
    MP4_GET4BYTES( i_reserved );
    MP4_GET2BYTES( p_gnre->genre );
    if( p_gnre->genre == 0 )
        MP4_READBOX_EXIT( 0 );
#ifdef MP4_VERBOSE
     printf( "read box: \"gnre\" genre=%i", p_gnre->i_genre );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_gnreFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_gnre_t *p_gnre;
    uint32_t i_data_len;
    uint32_t i_data_tag;
    uint32_t i_version;
    uint32_t i_reserved;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_gnre_t );

    p_gnre = p_box->data.p_gnre;

    MP4_GET4BYTES( i_data_len );
    MP4_GETFOURCC( i_data_tag );
    if( i_data_len < 10 || i_data_tag != ATOM_data )
        MP4_READBOX_EXIT( 0 );

    MP4_GET4BYTES( i_version );
    MP4_GET4BYTES( i_reserved );
    MP4_GET2BYTES( p_gnre->genre );
    if( p_gnre->genre == 0 )
        MP4_READBOX_EXIT( 0 );
#ifdef MP4_VERBOSE
     printf( "read box: \"gnre\" genre=%i", p_gnre->i_genre );
#endif

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_trkn( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_trkn_t *p_trkn;
    uint32_t i_data_len;
    uint32_t i_data_tag;
    uint32_t i_version;
    uint32_t i_reserved;

    MP4_READBOX_ENTER( mp4_box_data_trkn_t );

    p_trkn = p_box->data.p_trkn;

    MP4_GET4BYTES( i_data_len );
    MP4_GETFOURCC( i_data_tag );
    if( i_data_len < 12 || i_data_tag != ATOM_data )
        MP4_READBOX_EXIT( 0 );

    MP4_GET4BYTES( i_version );
    MP4_GET4BYTES( i_reserved );
    MP4_GET4BYTES( p_trkn->track_number );

// #ifdef MP4_VERBOSE
//      printf( "read box: \"trkn\" number=%i", p_trkn->track_number );
// #endif

    if( i_data_len > 15 )
    {
       MP4_GET4BYTES( p_trkn->track_total );

// #ifdef MP4_VERBOSE
//         printf( "read box: \"trkn\" total=%i", p_trkn->track_total );
// #endif

    }

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_trknFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
    mp4_box_data_trkn_t *p_trkn;
    uint32_t i_data_len;
    uint32_t i_data_tag;
    uint32_t i_version;
    uint32_t i_reserved;

    MP4_READBOX_ENTERFromBuffer( mp4_box_data_trkn_t );

    p_trkn = p_box->data.p_trkn;

    MP4_GET4BYTES( i_data_len );
    MP4_GETFOURCC( i_data_tag );
    if( i_data_len < 12 || i_data_tag != ATOM_data )
        MP4_READBOX_EXIT( 0 );

    MP4_GET4BYTES( i_version );
    MP4_GET4BYTES( i_reserved );
    MP4_GET4BYTES( p_trkn->track_number );

// #ifdef MP4_VERBOSE
//      printf( "read box: \"trkn\" number=%i", p_trkn->track_number );
// #endif

    if( i_data_len > 15 )
    {
       MP4_GET4BYTES( p_trkn->track_total );

// #ifdef MP4_VERBOSE
//         printf( "read box: \"trkn\" total=%i", p_trkn->track_total );
// #endif

    }

    MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_sample_soun( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_sample_soun_t );
   p_box->data.p_sample_soun->qt_description = NULL;

   /* Sanity check needed because the "wave" box does also contain an
   * "mp4a" box that we don't understand. */
   if( i_read < 28 )
   {
      i_read -= 30;
      MP4_READBOX_EXIT( 1 );
   }

   for( i = 0; i < 6 ; i++ )
   {
      MP4_GET1BYTE( p_box->data.p_sample_soun->reserved1[i] );
   }

   MP4_GET2BYTES( p_box->data.p_sample_soun->data_reference_index );

   /*
   * XXX hack -> produce a copy of the nearly complete chunk
   */
   p_box->data.p_sample_soun->qt_description = 0;
   p_box->data.p_sample_soun->p_qt_description = NULL;
   if( i_read > 0 )
   {
      p_box->data.p_sample_soun->p_qt_description = malloc( i_read );
      if( p_box->data.p_sample_soun->p_qt_description )
      {
         p_box->data.p_sample_soun->qt_description = i_read;
         memcpy( p_box->data.p_sample_soun->p_qt_description, p_peek, i_read );
      }
   }

   MP4_GET2BYTES( p_box->data.p_sample_soun->qt_version );
   MP4_GET2BYTES( p_box->data.p_sample_soun->qt_revision_level );
   MP4_GET4BYTES( p_box->data.p_sample_soun->qt_vendor );

   MP4_GET2BYTES( p_box->data.p_sample_soun->channelcount );
   MP4_GET2BYTES( p_box->data.p_sample_soun->samplesize );
   MP4_GET2BYTES( p_box->data.p_sample_soun->predefined );
   MP4_GET2BYTES( p_box->data.p_sample_soun->reserved3 );
   MP4_GET2BYTES( p_box->data.p_sample_soun->sampleratehi );
   MP4_GET2BYTES( p_box->data.p_sample_soun->sampleratelo );

   if( p_box->data.p_sample_soun->qt_version == 1 && i_read >= 16 )
   {
      /* SoundDescriptionV1 */
      MP4_GET4BYTES( p_box->data.p_sample_soun->sample_per_packet );
      MP4_GET4BYTES( p_box->data.p_sample_soun->bytes_per_packet );
      MP4_GET4BYTES( p_box->data.p_sample_soun->bytes_per_frame );
      MP4_GET4BYTES( p_box->data.p_sample_soun->bytes_per_sample );

#ifdef MP4_VERBOSE
       printf(
         "read box: \"soun\" qt3+ sample/packet=%d bytes/packet=%d "
         "bytes/frame=%d bytes/sample=%d",
         p_box->data.p_sample_soun->i_sample_per_packet,
         p_box->data.p_sample_soun->i_bytes_per_packet,
         p_box->data.p_sample_soun->i_bytes_per_frame,
         p_box->data.p_sample_soun->i_bytes_per_sample );
#endif
      stream_seek( p_stream, p_box->i_pos +
         mp4_box_headersize( p_box ) + 44, SEEK_SET );
   }
   else if( p_box->data.p_sample_soun->qt_version == 2 && i_read >= 36 )
   {
      /* SoundDescriptionV2 */
      double f_sample_rate;
      int64_t dummy;
      uint32_t i_channel;

      MP4_GET4BYTES( p_box->data.p_sample_soun->sample_per_packet );
      MP4_GET8BYTES( dummy );
      memcpy( &f_sample_rate, &dummy, 8 );

       printf( "read box: %f Hz", f_sample_rate );
      p_box->data.p_sample_soun->sampleratehi = (int)f_sample_rate % 65536;
      p_box->data.p_sample_soun->sampleratelo = f_sample_rate / 65536;

      MP4_GET4BYTES( i_channel );
      p_box->data.p_sample_soun->channelcount = i_channel;

#ifdef MP4_VERBOSE
       printf( "read box: \"soun\" V2" );
#endif
      stream_seek( p_stream, p_box->i_pos +
         mp4_box_headersize( p_box ) + 28 + 36, SEEK_SET );
   }
   else
   {
      p_box->data.p_sample_soun->sample_per_packet = 0;
      p_box->data.p_sample_soun->bytes_per_packet = 0;
      p_box->data.p_sample_soun->bytes_per_frame = 0;
      p_box->data.p_sample_soun->bytes_per_sample = 0;

#ifdef MP4_VERBOSE
       printf( "read box: \"soun\" mp4 or qt1/2 (rest=%"PRId64")",
         i_read );
#endif
      stream_seek( p_stream, p_box->i_pos +
         mp4_box_headersize( p_box ) + 28, SEEK_SET );
   }

   if( p_box->i_type == ATOM_drms )
   {
      assert(0);
      //         char *home = config_GetUserDir( VLC_HOME_DIR );
      //         if( home != NULL )
      //         {
      //             p_box->data.p_sample_soun->p_drms = drms_alloc( home );
      //             if( p_box->data.p_sample_soun->p_drms == NULL )
      //                 msg_Err( p_stream, "drms_alloc() failed" );
      //         }
   }

   if( p_box->i_type == ATOM_samr || p_box->i_type == ATOM_sawb )
   {
      /* Ignore channelcount for AMR (3gpp AMRSpecificBox) */
      p_box->data.p_sample_soun->channelcount = 1;
   }

   MP4_ReadBoxContainerRaw( p_stream, p_box ); /* esds/wave/... */

#ifdef MP4_VERBOSE
    printf( "read box: \"soun\" in stsd channel %d "
      "sample size %d sample rate %f",
      p_box->data.p_sample_soun->channelcount,
      p_box->data.p_sample_soun->samplesize,
      (float)p_box->data.p_sample_soun->sampleratehi +
      (float)p_box->data.p_sample_soun->sampleratelo / 65536 );

#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_sample_sounFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_sample_soun_t );
   p_box->data.p_sample_soun->qt_description = NULL;

   /* Sanity check needed because the "wave" box does also contain an
   * "mp4a" box that we don't understand. */
   if( i_read < 28 )
   {
      i_read -= 30;
      MP4_READBOX_EXIT( 1 );
   }

   for( i = 0; i < 6 ; i++ )
   {
      MP4_GET1BYTE( p_box->data.p_sample_soun->reserved1[i] );
   }

   MP4_GET2BYTES( p_box->data.p_sample_soun->data_reference_index );

   /*
   * XXX hack -> produce a copy of the nearly complete chunk
   */
   p_box->data.p_sample_soun->qt_description = 0;
   p_box->data.p_sample_soun->p_qt_description = NULL;
   if( i_read > 0 )
   {
      p_box->data.p_sample_soun->p_qt_description = malloc( i_read );
      if( p_box->data.p_sample_soun->p_qt_description )
      {
         p_box->data.p_sample_soun->qt_description = i_read;
         memcpy( p_box->data.p_sample_soun->p_qt_description, p_peek, i_read );
      }
   }

   MP4_GET2BYTES( p_box->data.p_sample_soun->qt_version );
   MP4_GET2BYTES( p_box->data.p_sample_soun->qt_revision_level );
   MP4_GET4BYTES( p_box->data.p_sample_soun->qt_vendor );

   MP4_GET2BYTES( p_box->data.p_sample_soun->channelcount );
   MP4_GET2BYTES( p_box->data.p_sample_soun->samplesize );
   MP4_GET2BYTES( p_box->data.p_sample_soun->predefined );
   MP4_GET2BYTES( p_box->data.p_sample_soun->reserved3 );
   MP4_GET2BYTES( p_box->data.p_sample_soun->sampleratehi );
   MP4_GET2BYTES( p_box->data.p_sample_soun->sampleratelo );

   if( p_box->data.p_sample_soun->qt_version == 1 && i_read >= 16 )
   {
      /* SoundDescriptionV1 */
      MP4_GET4BYTES( p_box->data.p_sample_soun->sample_per_packet );
      MP4_GET4BYTES( p_box->data.p_sample_soun->bytes_per_packet );
      MP4_GET4BYTES( p_box->data.p_sample_soun->bytes_per_frame );
      MP4_GET4BYTES( p_box->data.p_sample_soun->bytes_per_sample );

#ifdef MP4_VERBOSE
       printf(
         "read box: \"soun\" qt3+ sample/packet=%d bytes/packet=%d "
         "bytes/frame=%d bytes/sample=%d",
         p_box->data.p_sample_soun->i_sample_per_packet,
         p_box->data.p_sample_soun->i_bytes_per_packet,
         p_box->data.p_sample_soun->i_bytes_per_frame,
         p_box->data.p_sample_soun->i_bytes_per_sample );
#endif
      buffer_seek( p_stream,\
         mp4_box_headersize( p_box ) + 44, p_box->i_pos );
   }
   else if( p_box->data.p_sample_soun->qt_version == 2 && i_read >= 36 )
   {
      /* SoundDescriptionV2 */
      double f_sample_rate;
      int64_t dummy;
      uint32_t i_channel;

      MP4_GET4BYTES( p_box->data.p_sample_soun->sample_per_packet );
      MP4_GET8BYTES( dummy );
      memcpy( &f_sample_rate, &dummy, 8 );

       printf( "read box: %f Hz", f_sample_rate );
      p_box->data.p_sample_soun->sampleratehi = (int)f_sample_rate % 65536;
      p_box->data.p_sample_soun->sampleratelo = f_sample_rate / 65536;

      MP4_GET4BYTES( i_channel );
      p_box->data.p_sample_soun->channelcount = i_channel;

#ifdef MP4_VERBOSE
       printf( "read box: \"soun\" V2" );
#endif
      buffer_seek( p_stream,\
         mp4_box_headersize( p_box ) + 28 + 36, p_box->i_pos );
   }
   else
   {
      p_box->data.p_sample_soun->sample_per_packet = 0;
      p_box->data.p_sample_soun->bytes_per_packet = 0;
      p_box->data.p_sample_soun->bytes_per_frame = 0;
      p_box->data.p_sample_soun->bytes_per_sample = 0;

#ifdef MP4_VERBOSE
       printf( "read box: \"soun\" mp4 or qt1/2 (rest=%"PRId64")",
         i_read );
#endif
      buffer_seek( p_stream,\
         mp4_box_headersize( p_box ) + 28, p_box->i_pos );
   }

   if( p_box->i_type == ATOM_drms )
   {
      assert(0);
      //         char *home = config_GetUserDir( VLC_HOME_DIR );
      //         if( home != NULL )
      //         {
      //             p_box->data.p_sample_soun->p_drms = drms_alloc( home );
      //             if( p_box->data.p_sample_soun->p_drms == NULL )
      //                 msg_Err( p_stream, "drms_alloc() failed" );
      //         }
   }

   if( p_box->i_type == ATOM_samr || p_box->i_type == ATOM_sawb )
   {
      /* Ignore channelcount for AMR (3gpp AMRSpecificBox) */
      p_box->data.p_sample_soun->channelcount = 1;
   }

   MP4_ReadBoxContainerRawFromBuffer( p_stream, p_box ); /* esds/wave/... */

#ifdef MP4_VERBOSE
    printf( "read box: \"soun\" in stsd channel %d "
      "sample size %d sample rate %f",
      p_box->data.p_sample_soun->channelcount,
      p_box->data.p_sample_soun->samplesize,
      (float)p_box->data.p_sample_soun->sampleratehi +
      (float)p_box->data.p_sample_soun->sampleratelo / 65536 );

#endif
   MP4_READBOX_EXIT( 1 );
}
/*****************************************************************************
* aes_s: AES keys structure
*****************************************************************************
* This structure stores a set of keys usable for encryption and decryption
* with the AES/Rijndael algorithm.
*****************************************************************************/
#define AES_KEY_COUNT 10
#define PATH_MAX 260
struct aes_s
{
   uint32_t pp_enc_keys[ AES_KEY_COUNT + 1 ][ 4 ];
   uint32_t pp_dec_keys[ AES_KEY_COUNT + 1 ][ 4 ];
};

/*****************************************************************************
* drms_s: DRMS structure
*****************************************************************************
* This structure stores the static information needed to decrypt DRMS data.
*****************************************************************************/
struct drms_s
{
   uint32_t i_user;
   uint32_t i_key;
   uint8_t  p_iviv[ 16 ];
   uint8_t *p_name;

   uint32_t p_key[ 4 ];
   struct aes_s aes;

   char     psz_homedir[ PATH_MAX ];
};

/*****************************************************************************
* drms_free: free a previously allocated DRMS structure
*****************************************************************************/
void drms_free( void *_p_drms )
{
   struct drms_s *p_drms = (struct drms_s *)_p_drms;

   //free( (void *)p_drms->p_name );
   free( p_drms->p_name );
   free( p_drms );
}


static void MP4_FreeBox_sample_soun( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_sample_soun->p_qt_description );

   if( p_box->i_type == ATOM_drms )
   {
      if( p_box->data.p_sample_soun->drms )
      {
         drms_free( p_box->data.p_sample_soun->drms );
      }
   }
}


int MP4_ReadBox_sample_vide( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_sample_vide_t );

   for( i = 0; i < 6 ; i++ )
   {
      MP4_GET1BYTE( p_box->data.p_sample_vide->reserved1[i] );
   }

   MP4_GET2BYTES( p_box->data.p_sample_vide->data_reference_index );

   /*
   * XXX hack -> produce a copy of the nearly complete chunk
   */
   if( i_read > 0 )
   {
      p_box->data.p_sample_vide->p_qt_image_description = malloc( i_read );
      if( unlikely( p_box->data.p_sample_vide->p_qt_image_description == NULL ) )
         MP4_READBOX_EXIT( 0 );
      p_box->data.p_sample_vide->qt_image_description = i_read;
      memcpy( p_box->data.p_sample_vide->p_qt_image_description,
         p_peek, i_read );
   }
   else
   {
      p_box->data.p_sample_vide->qt_image_description = 0;
      p_box->data.p_sample_vide->p_qt_image_description = NULL;
   }

   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_version );
   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_revision_level );
   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_vendor );

   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_temporal_quality );
   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_spatial_quality );

   MP4_GET2BYTES( p_box->data.p_sample_vide->width );
   MP4_GET2BYTES( p_box->data.p_sample_vide->height );

   MP4_GET4BYTES( p_box->data.p_sample_vide->horizresolution );
   MP4_GET4BYTES( p_box->data.p_sample_vide->vertresolution );

   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_data_size );
   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_frame_count );

   memcpy( &p_box->data.p_sample_vide->compressorname, p_peek, 32 );
   p_peek += 32; i_read -= 32;

   MP4_GET2BYTES( p_box->data.p_sample_vide->depth );
   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_color_table );

   stream_seek( p_stream, p_box->i_pos + mp4_box_headersize( p_box ) + 78, SEEK_SET );

   if( p_box->i_type == ATOM_drmi )
   {
      assert(0);
      //         char *home = config_GetUserDir( VLC_HOME_DIR );
      //         if( home != NULL )
      //         {
      //             p_box->data.p_sample_vide->p_drms = drms_alloc( home );
      //             if( p_box->data.p_sample_vide->p_drms == NULL )
      //                 msg_Err( p_stream, "drms_alloc() failed" );
      //         }
   }

   MP4_ReadBoxContainerRaw( p_stream, p_box );

#ifdef MP4_VERBOSE
    printf( "read box: \"vide\" in stsd %dx%d depth %d",
      p_box->data.p_sample_vide->width,
      p_box->data.p_sample_vide->height,
      p_box->data.p_sample_vide->depth );

#endif

   MP4_READBOX_EXIT( 1 );
}


int MP4_ReadBox_sample_videFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_sample_vide_t );

   for( i = 0; i < 6 ; i++ )
   {
      MP4_GET1BYTE( p_box->data.p_sample_vide->reserved1[i] );
   }

   MP4_GET2BYTES( p_box->data.p_sample_vide->data_reference_index );

   /*
   * XXX hack -> produce a copy of the nearly complete chunk
   */
   if( i_read > 0 )
   {
      p_box->data.p_sample_vide->p_qt_image_description = malloc( i_read );
      if( unlikely( p_box->data.p_sample_vide->p_qt_image_description == NULL ) )
         MP4_READBOX_EXIT( 0 );
      p_box->data.p_sample_vide->qt_image_description = i_read;
      memcpy( p_box->data.p_sample_vide->p_qt_image_description,
         p_peek, i_read );
   }
   else
   {
      p_box->data.p_sample_vide->qt_image_description = 0;
      p_box->data.p_sample_vide->p_qt_image_description = NULL;
   }

   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_version );
   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_revision_level );
   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_vendor );

   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_temporal_quality );
   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_spatial_quality );

   MP4_GET2BYTES( p_box->data.p_sample_vide->width );
   MP4_GET2BYTES( p_box->data.p_sample_vide->height );

   MP4_GET4BYTES( p_box->data.p_sample_vide->horizresolution );
   MP4_GET4BYTES( p_box->data.p_sample_vide->vertresolution );

   MP4_GET4BYTES( p_box->data.p_sample_vide->qt_data_size );
   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_frame_count );

   memcpy( &p_box->data.p_sample_vide->compressorname, p_peek, 32 );
   p_peek += 32; i_read -= 32;

   MP4_GET2BYTES( p_box->data.p_sample_vide->depth );
   MP4_GET2BYTES( p_box->data.p_sample_vide->qt_color_table );

   buffer_seek( p_stream, mp4_box_headersize( p_box ) + 78, p_box->i_pos );

   if( p_box->i_type == ATOM_drmi )
   {
      assert(0);
      //         char *home = config_GetUserDir( VLC_HOME_DIR );
      //         if( home != NULL )
      //         {
      //             p_box->data.p_sample_vide->p_drms = drms_alloc( home );
      //             if( p_box->data.p_sample_vide->p_drms == NULL )
      //                 msg_Err( p_stream, "drms_alloc() failed" );
      //         }
   }

   MP4_ReadBoxContainerRawFromBuffer( p_stream, p_box );

#ifdef MP4_VERBOSE
    printf( "read box: \"vide\" in stsd %dx%d depth %d",
      p_box->data.p_sample_vide->width,
      p_box->data.p_sample_vide->height,
      p_box->data.p_sample_vide->depth );

#endif

   MP4_READBOX_EXIT( 1 );
}


void MP4_FreeBox_sample_vide( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_sample_vide->p_qt_image_description );

   if( p_box->i_type == ATOM_drmi )
   {
      if( p_box->data.p_sample_vide->drms )
      {
         drms_free( p_box->data.p_sample_vide->drms );
      }
   }
}


int MP4_ReadBox_sample_mmth( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   unsigned int  mmth_buf;
   int j;
   MP4_READBOX_ENTER( mp4_box_data_sample_mmth_t );

   for( i = 0; i < 6 ; i++ )
   {
      MP4_GET1BYTE( p_box->data.p_sample_mmth->reserved1[i] );
   }

   MP4_GET2BYTES( p_box->data.p_sample_mmth->data_reference_index );


   MP4_GET2BYTES( p_box->data.p_sample_mmth->hinttrackversion );
   MP4_GET2BYTES( p_box->data.p_sample_mmth->highestcompatibleversion );
   MP4_GET2BYTES( p_box->data.p_sample_mmth->packet_id );
   MP4_GET1BYTE(mmth_buf);
   p_box->data.p_sample_mmth->has_mfus_flag=(mmth_buf>>7)&0x01;
   p_box->data.p_sample_mmth->is_timed=(mmth_buf>>6)&0x01;
   p_box->data.p_sample_mmth->reserved=mmth_buf&0x3F;

   /*MP4_GETFOURCC( p_box->data.p_sample_mmth->asset_id_scheme );
   MP4_GET4BYTES( p_box->data.p_sample_mmth->asset_id_length );
   p_box->data.p_sample_mmth->asset_id_value=(char *)malloc(p_box->data.p_sample_mmth->asset_id_length);
   for (j=0;j<p_box->data.p_sample_mmth->asset_id_length;j++)
	  {

	   MP4_GET1BYTE( p_box->data.p_sample_mmth->asset_id_value[j] );
	  }*/

   MP4_READBOX_EXIT( 1 );
}


int MP4_ReadBox_sample_mmthFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   unsigned int  mmth_buf;
   int j;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_sample_mmth_t );

   for( i = 0; i < 6 ; i++ )
   {
      MP4_GET1BYTE( p_box->data.p_sample_mmth->reserved1[i] );
   }

   MP4_GET2BYTES( p_box->data.p_sample_mmth->data_reference_index );


   MP4_GET2BYTES( p_box->data.p_sample_mmth->hinttrackversion );
   MP4_GET2BYTES( p_box->data.p_sample_mmth->highestcompatibleversion );
   MP4_GET2BYTES( p_box->data.p_sample_mmth->packet_id );
   MP4_GET1BYTE(mmth_buf);
   p_box->data.p_sample_mmth->has_mfus_flag=(mmth_buf>>7)&0x01;
   p_box->data.p_sample_mmth->is_timed=(mmth_buf>>6)&0x01;
   p_box->data.p_sample_mmth->reserved=mmth_buf&0x3F;

   /*MP4_GETFOURCC( p_box->data.p_sample_mmth->asset_id_scheme );
   MP4_GET4BYTES( p_box->data.p_sample_mmth->asset_id_length );
   p_box->data.p_sample_mmth->asset_id_value=(char *)malloc(p_box->data.p_sample_mmth->asset_id_length);
   for (j=0;j<p_box->data.p_sample_mmth->asset_id_length;j++)
	  {

	   MP4_GET1BYTE( p_box->data.p_sample_mmth->asset_id_value[j] );
	  }*/

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_sample_mp4s( stream_t *p_stream, mp4_box_t *p_box )
{
   stream_seek( p_stream, p_box->i_pos + mp4_box_headersize( p_box ) + 8, SEEK_SET );
   MP4_ReadBoxContainerRaw( p_stream, p_box );
   return 1;
}

static int MP4_ReadBox_sample_text( stream_t *p_stream, mp4_box_t *p_box )
{
   int32_t t;

   MP4_READBOX_ENTER( mp4_box_data_sample_text_t );

   MP4_GET4BYTES( p_box->data.p_sample_text->reserved1 );
   MP4_GET2BYTES( p_box->data.p_sample_text->reserved2 );

   MP4_GET2BYTES( p_box->data.p_sample_text->data_reference_index );

   MP4_GET4BYTES( p_box->data.p_sample_text->display_flags );

   MP4_GET4BYTES( t );
   switch( t )
   {
      /* FIXME search right signification */
   case 1: // Center
      p_box->data.p_sample_text->justification_horizontal = 1;
      p_box->data.p_sample_text->justification_vertical = 1;
      break;
   case -1:    // Flush Right
      p_box->data.p_sample_text->justification_horizontal = -1;
      p_box->data.p_sample_text->justification_vertical = -1;
      break;
   case -2:    // Flush p_first
      p_box->data.p_sample_text->justification_horizontal = 0;
      p_box->data.p_sample_text->justification_vertical = 0;
      break;
   case 0: // Flush Default
   default:
      p_box->data.p_sample_text->justification_horizontal = 1;
      p_box->data.p_sample_text->justification_vertical = -1;
      break;
   }

   MP4_GET2BYTES( p_box->data.p_sample_text->background_color[0] );
   MP4_GET2BYTES( p_box->data.p_sample_text->background_color[1] );
   MP4_GET2BYTES( p_box->data.p_sample_text->background_color[2] );
   p_box->data.p_sample_text->background_color[3] = 0;

   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_top );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_left );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_bottom );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_right );

#ifdef MP4_VERBOSE
    printf( "read box: \"text\" in stsd text" );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_sample_textFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   int32_t t;

   MP4_READBOX_ENTERFromBuffer( mp4_box_data_sample_text_t );

   MP4_GET4BYTES( p_box->data.p_sample_text->reserved1 );
   MP4_GET2BYTES( p_box->data.p_sample_text->reserved2 );

   MP4_GET2BYTES( p_box->data.p_sample_text->data_reference_index );

   MP4_GET4BYTES( p_box->data.p_sample_text->display_flags );

   MP4_GET4BYTES( t );
   switch( t )
   {
      /* FIXME search right signification */
   case 1: // Center
      p_box->data.p_sample_text->justification_horizontal = 1;
      p_box->data.p_sample_text->justification_vertical = 1;
      break;
   case -1:    // Flush Right
      p_box->data.p_sample_text->justification_horizontal = -1;
      p_box->data.p_sample_text->justification_vertical = -1;
      break;
   case -2:    // Flush p_first
      p_box->data.p_sample_text->justification_horizontal = 0;
      p_box->data.p_sample_text->justification_vertical = 0;
      break;
   case 0: // Flush Default
   default:
      p_box->data.p_sample_text->justification_horizontal = 1;
      p_box->data.p_sample_text->justification_vertical = -1;
      break;
   }

   MP4_GET2BYTES( p_box->data.p_sample_text->background_color[0] );
   MP4_GET2BYTES( p_box->data.p_sample_text->background_color[1] );
   MP4_GET2BYTES( p_box->data.p_sample_text->background_color[2] );
   p_box->data.p_sample_text->background_color[3] = 0;

   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_top );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_left );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_bottom );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_right );

#ifdef MP4_VERBOSE
    printf( "read box: \"text\" in stsd text" );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_sample_tx3g( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_sample_text_t );

   MP4_GET4BYTES( p_box->data.p_sample_text->reserved1 );
   MP4_GET2BYTES( p_box->data.p_sample_text->reserved2 );

   MP4_GET2BYTES( p_box->data.p_sample_text->data_reference_index );

   MP4_GET4BYTES( p_box->data.p_sample_text->display_flags );

   MP4_GET1BYTE ( p_box->data.p_sample_text->justification_horizontal );
   MP4_GET1BYTE ( p_box->data.p_sample_text->justification_vertical );

   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[0] );
   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[1] );
   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[2] );
   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[3] );

   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_top );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_left );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_bottom );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_right );

#ifdef MP4_VERBOSE
    printf( "read box: \"tx3g\" in stsd text" );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_sample_tx3gFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_sample_text_t );

   MP4_GET4BYTES( p_box->data.p_sample_text->reserved1 );
   MP4_GET2BYTES( p_box->data.p_sample_text->reserved2 );

   MP4_GET2BYTES( p_box->data.p_sample_text->data_reference_index );

   MP4_GET4BYTES( p_box->data.p_sample_text->display_flags );

   MP4_GET1BYTE ( p_box->data.p_sample_text->justification_horizontal );
   MP4_GET1BYTE ( p_box->data.p_sample_text->justification_vertical );

   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[0] );
   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[1] );
   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[2] );
   MP4_GET1BYTE ( p_box->data.p_sample_text->background_color[3] );

   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_top );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_left );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_bottom );
   MP4_GET2BYTES( p_box->data.p_sample_text->text_box_right );

#ifdef MP4_VERBOSE
    printf( "read box: \"tx3g\" in stsd text" );
#endif

   MP4_READBOX_EXIT( 1 );
}


#if 0
/* We can't easily call it, and anyway ~ 20 bytes lost isn't a real problem */
static void MP4_FreeBox_sample_text( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_sample_text->psz_text_name );
}
#endif


static int MP4_ReadBox_stsd( stream_t *p_stream, mp4_box_t *p_box )
{

   MP4_READBOX_ENTER( mp4_box_data_stsd_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsd );

   MP4_GET4BYTES( p_box->data.p_stsd->entry_count );

   stream_seek( p_stream, p_box->i_pos + mp4_box_headersize( p_box ) + 8, SEEK_SET );

   MP4_ReadBoxContainerRaw( p_stream, p_box );

#ifdef MP4_VERBOSE
    printf( "read box: \"stsd\" entry-count %d",
      p_box->data.p_stsd->entry_count );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stsdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{

   MP4_READBOX_ENTER( mp4_box_data_stsd_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsd );

   MP4_GET4BYTES( p_box->data.p_stsd->entry_count );

   buffer_seek( p_stream, mp4_box_headersize( p_box ) + 8, p_box->i_pos );

   MP4_ReadBoxContainerRawFromBuffer( p_stream, p_box );

#ifdef MP4_VERBOSE
    printf( "read box: \"stsd\" entry-count %d",
      p_box->data.p_stsd->entry_count );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stsz( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_stsz_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsz );

   MP4_GET4BYTES( p_box->data.p_stsz->sample_size );
   MP4_GET4BYTES( p_box->data.p_stsz->sample_count );

   if( p_box->data.p_stsz->sample_size == 0 )
   {
      p_box->data.p_stsz->entry_size =
         calloc( p_box->data.p_stsz->sample_count, sizeof(uint32_t) );
      if( unlikely( !p_box->data.p_stsz->entry_size ) )
         MP4_READBOX_EXIT( 0 );

      for( i = 0; (i<p_box->data.p_stsz->sample_count)&&(i_read >= 4 ); i++ )
      {
         MP4_GET4BYTES( p_box->data.p_stsz->entry_size[i] );
      }
   }
   else
      p_box->data.p_stsz->entry_size = NULL;

#ifdef MP4_VERBOSE
    printf( "read box: \"stsz\" sample-size %d sample-count %d",
      p_box->data.p_stsz->sample_size,
      p_box->data.p_stsz->sample_count );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stszFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_stsz_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsz );

   MP4_GET4BYTES( p_box->data.p_stsz->sample_size );
   MP4_GET4BYTES( p_box->data.p_stsz->sample_count );

   if( p_box->data.p_stsz->sample_size == 0 )
   {
      p_box->data.p_stsz->entry_size =
         calloc( p_box->data.p_stsz->sample_count, sizeof(uint32_t) );
      if( unlikely( !p_box->data.p_stsz->entry_size ) )
         MP4_READBOX_EXIT( 0 );

      for( i = 0; (i<p_box->data.p_stsz->sample_count)&&(i_read >= 4 ); i++ )
      {
         MP4_GET4BYTES( p_box->data.p_stsz->entry_size[i] );
      }
   }
   else
      p_box->data.p_stsz->entry_size = NULL;

#ifdef MP4_VERBOSE
    printf( "read box: \"stsz\" sample-size %d sample-count %d",
      p_box->data.p_stsz->sample_size,
      p_box->data.p_stsz->sample_count );
#endif

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_stsz( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_stsz->entry_size );
}

static void MP4_FreeBox_stsc( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_stsc->first_chunk );
   FREENULL( p_box->data.p_stsc->samples_per_chunk );
   FREENULL( p_box->data.p_stsc->sample_description_index );
}

static int MP4_ReadBox_stsc( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_stsc_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsc );

   MP4_GET4BYTES( p_box->data.p_stsc->entry_count );

   p_box->data.p_stsc->first_chunk =
      calloc( p_box->data.p_stsc->entry_count, sizeof(uint32_t) );
   p_box->data.p_stsc->samples_per_chunk =
      calloc( p_box->data.p_stsc->entry_count, sizeof(uint32_t) );
   p_box->data.p_stsc->sample_description_index =
      calloc( p_box->data.p_stsc->entry_count, sizeof(uint32_t) );
   if( unlikely( p_box->data.p_stsc->first_chunk == NULL
      || p_box->data.p_stsc->samples_per_chunk == NULL
      || p_box->data.p_stsc->sample_description_index == NULL ) )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_stsc->entry_count )&&( i_read >= 12 );i++ )
   {
      MP4_GET4BYTES( p_box->data.p_stsc->first_chunk[i] );
      MP4_GET4BYTES( p_box->data.p_stsc->samples_per_chunk[i] );
      MP4_GET4BYTES( p_box->data.p_stsc->sample_description_index[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stsc\" entry-count %d",
      p_box->data.p_stsc->entry_count );

#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stscFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_stsc_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsc );

   MP4_GET4BYTES( p_box->data.p_stsc->entry_count );

   p_box->data.p_stsc->first_chunk =
      calloc( p_box->data.p_stsc->entry_count, sizeof(uint32_t) );
   p_box->data.p_stsc->samples_per_chunk =
      calloc( p_box->data.p_stsc->entry_count, sizeof(uint32_t) );
   p_box->data.p_stsc->sample_description_index =
      calloc( p_box->data.p_stsc->entry_count, sizeof(uint32_t) );
   if( unlikely( p_box->data.p_stsc->first_chunk == NULL
      || p_box->data.p_stsc->samples_per_chunk == NULL
      || p_box->data.p_stsc->sample_description_index == NULL ) )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_stsc->entry_count )&&( i_read >= 12 );i++ )
   {
      MP4_GET4BYTES( p_box->data.p_stsc->first_chunk[i] );
      MP4_GET4BYTES( p_box->data.p_stsc->samples_per_chunk[i] );
      MP4_GET4BYTES( p_box->data.p_stsc->sample_description_index[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stsc\" entry-count %d",
      p_box->data.p_stsc->entry_count );

#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stco_co64( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_co64_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_co64 );

   MP4_GET4BYTES( p_box->data.p_co64->entry_count );

   p_box->data.p_co64->chunk_offset =
      calloc( p_box->data.p_co64->entry_count, sizeof(uint64_t) );
   if( p_box->data.p_co64->chunk_offset == NULL )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < p_box->data.p_co64->entry_count; i++ )
   {
      if( p_box->i_type == ATOM_stco )
      {
         if( i_read < 4 )
         {
            break;
         }
         MP4_GET4BYTES( p_box->data.p_co64->chunk_offset[i] );
      }
      else
      {
         if( i_read < 8 )
         {
            break;
         }
         MP4_GET8BYTES( p_box->data.p_co64->chunk_offset[i] );
      }
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"co64\" entry-count %d",
      p_box->data.p_co64->entry_count );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stco_co64FromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_co64_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_co64 );

   MP4_GET4BYTES( p_box->data.p_co64->entry_count );

   p_box->data.p_co64->chunk_offset =
      calloc( p_box->data.p_co64->entry_count, sizeof(uint64_t) );
   if( p_box->data.p_co64->chunk_offset == NULL )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < p_box->data.p_co64->entry_count; i++ )
   {
      if( p_box->i_type == ATOM_stco )
      {
         if( i_read < 4 )
         {
            break;
         }
         MP4_GET4BYTES( p_box->data.p_co64->chunk_offset[i] );
      }
      else
      {
         if( i_read < 8 )
         {
            break;
         }
         MP4_GET8BYTES( p_box->data.p_co64->chunk_offset[i] );
      }
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"co64\" entry-count %d",
      p_box->data.p_co64->entry_count );
#endif

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_stco_co64( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_co64->chunk_offset );
}

static int MP4_ReadBox_stss( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_stss_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stss );

   MP4_GET4BYTES( p_box->data.p_stss->entry_count );

   p_box->data.p_stss->sample_number =
      calloc( p_box->data.p_stss->entry_count, sizeof(uint32_t) );
   if( unlikely( p_box->data.p_stss->sample_number == NULL ) )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; (i < p_box->data.p_stss->entry_count )&&( i_read >= 4 ); i++ )
   {

      MP4_GET4BYTES( p_box->data.p_stss->sample_number[i] );
      /* XXX in libmp4 sample begin at 0 */
      p_box->data.p_stss->sample_number[i]--;
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stss\" entry-count %d",
      p_box->data.p_stss->i_entry_count );

#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stssFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_stss_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stss );

   MP4_GET4BYTES( p_box->data.p_stss->entry_count );

   p_box->data.p_stss->sample_number =
      calloc( p_box->data.p_stss->entry_count, sizeof(uint32_t) );
   if( unlikely( p_box->data.p_stss->sample_number == NULL ) )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; (i < p_box->data.p_stss->entry_count )&&( i_read >= 4 ); i++ )
   {

      MP4_GET4BYTES( p_box->data.p_stss->sample_number[i] );
      /* XXX in libmp4 sample begin at 0 */
      p_box->data.p_stss->sample_number[i]--;
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stss\" entry-count %d",
      p_box->data.p_stss->i_entry_count );

#endif
   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_stss( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_stss->sample_number );
}

static void MP4_FreeBox_stsh( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_stsh->shadowed_sample_number );
   FREENULL( p_box->data.p_stsh->sync_sample_number );
}

static int MP4_ReadBox_stsh( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_stsh_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsh );


   MP4_GET4BYTES( p_box->data.p_stsh->entry_count );

   p_box->data.p_stsh->shadowed_sample_number =
      calloc( p_box->data.p_stsh->entry_count, sizeof(uint32_t) );
   p_box->data.p_stsh->sync_sample_number =
      calloc( p_box->data.p_stsh->entry_count, sizeof(uint32_t) );

   if( p_box->data.p_stsh->shadowed_sample_number == NULL
      || p_box->data.p_stsh->sync_sample_number == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_stss->entry_count )&&( i_read >= 8 ); i++ )
   {
      MP4_GET4BYTES( p_box->data.p_stsh->shadowed_sample_number[i] );
      MP4_GET4BYTES( p_box->data.p_stsh->sync_sample_number[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stsh\" entry-count %d",
      p_box->data.p_stsh->i_entry_count );
#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_stshFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_stsh_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stsh );


   MP4_GET4BYTES( p_box->data.p_stsh->entry_count );

   p_box->data.p_stsh->shadowed_sample_number =
      calloc( p_box->data.p_stsh->entry_count, sizeof(uint32_t) );
   p_box->data.p_stsh->sync_sample_number =
      calloc( p_box->data.p_stsh->entry_count, sizeof(uint32_t) );

   if( p_box->data.p_stsh->shadowed_sample_number == NULL
      || p_box->data.p_stsh->sync_sample_number == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; (i < p_box->data.p_stss->entry_count )&&( i_read >= 8 ); i++ )
   {
      MP4_GET4BYTES( p_box->data.p_stsh->shadowed_sample_number[i] );
      MP4_GET4BYTES( p_box->data.p_stsh->sync_sample_number[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stsh\" entry-count %d",
      p_box->data.p_stsh->i_entry_count );
#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stdp( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_stdp_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stdp );

   p_box->data.p_stdp->priority =
      calloc( i_read / 2, sizeof(uint16_t) );

   if( unlikely( !p_box->data.p_stdp->priority ) )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < i_read / 2 ; i++ )
   {
      MP4_GET2BYTES( p_box->data.p_stdp->priority[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stdp\" entry-count %"PRId64,
      i_read / 2 );

#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_stdpFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_stdp_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_stdp );

   p_box->data.p_stdp->priority =
      calloc( i_read / 2, sizeof(uint16_t) );

   if( unlikely( !p_box->data.p_stdp->priority ) )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < i_read / 2 ; i++ )
   {
      MP4_GET2BYTES( p_box->data.p_stdp->priority[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stdp\" entry-count %"PRId64,
      i_read / 2 );

#endif
   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_stdp( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_stdp->priority );
}

static void MP4_FreeBox_padb( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_padb->reserved1 );
   FREENULL( p_box->data.p_padb->pad2 );
   FREENULL( p_box->data.p_padb->reserved2 );
   FREENULL( p_box->data.p_padb->pad1 );
}

static int MP4_ReadBox_padb( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t count;
   unsigned int i = 0;

   MP4_READBOX_ENTER( mp4_box_data_padb_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_padb );

   MP4_GET4BYTES( p_box->data.p_padb->sample_count );
   count = (p_box->data.p_padb->sample_count + 1) / 2;

   p_box->data.p_padb->reserved1 = calloc( count, sizeof(uint16_t) );
   p_box->data.p_padb->pad2 = calloc( count, sizeof(uint16_t) );
   p_box->data.p_padb->reserved2 = calloc( count, sizeof(uint16_t) );
   p_box->data.p_padb->pad1 = calloc( count, sizeof(uint16_t) );
   if( p_box->data.p_padb->reserved1 == NULL
      || p_box->data.p_padb->pad2 == NULL
      || p_box->data.p_padb->reserved2 == NULL
      || p_box->data.p_padb->pad1 == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; i < i_read / 2 ; i++ )
   {
      if( i >= count )
      {
         MP4_READBOX_EXIT( 0 );
      }
      p_box->data.p_padb->reserved1[i] = ( (*p_peek) >> 7 )&0x01;
      p_box->data.p_padb->pad2[i] = ( (*p_peek) >> 4 )&0x07;
      p_box->data.p_padb->reserved1[i] = ( (*p_peek) >> 3 )&0x01;
      p_box->data.p_padb->pad1[i] = ( (*p_peek) )&0x07;

      p_peek += 1; i_read -= 1;
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stdp\" entry-count %"PRId64,
      i_read / 2 );

#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_padbFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t count;
   unsigned int i = 0;

   MP4_READBOX_ENTERFromBuffer( mp4_box_data_padb_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_padb );

   MP4_GET4BYTES( p_box->data.p_padb->sample_count );
   count = (p_box->data.p_padb->sample_count + 1) / 2;

   p_box->data.p_padb->reserved1 = calloc( count, sizeof(uint16_t) );
   p_box->data.p_padb->pad2 = calloc( count, sizeof(uint16_t) );
   p_box->data.p_padb->reserved2 = calloc( count, sizeof(uint16_t) );
   p_box->data.p_padb->pad1 = calloc( count, sizeof(uint16_t) );
   if( p_box->data.p_padb->reserved1 == NULL
      || p_box->data.p_padb->pad2 == NULL
      || p_box->data.p_padb->reserved2 == NULL
      || p_box->data.p_padb->pad1 == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }

   for( i = 0; i < i_read / 2 ; i++ )
   {
      if( i >= count )
      {
         MP4_READBOX_EXIT( 0 );
      }
      p_box->data.p_padb->reserved1[i] = ( (*p_peek) >> 7 )&0x01;
      p_box->data.p_padb->pad2[i] = ( (*p_peek) >> 4 )&0x07;
      p_box->data.p_padb->reserved1[i] = ( (*p_peek) >> 3 )&0x01;
      p_box->data.p_padb->pad1[i] = ( (*p_peek) )&0x07;

      p_peek += 1; i_read -= 1;
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"stdp\" entry-count %"PRId64,
      i_read / 2 );

#endif
   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_elst( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_elst->segment_duration );
   FREENULL( p_box->data.p_elst->media_time );
   FREENULL( p_box->data.p_elst->media_rate_integer );
   FREENULL( p_box->data.p_elst->media_rate_fraction );
}

static int MP4_ReadBox_elst( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTER( mp4_box_data_elst_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_elst );


   MP4_GET4BYTES( p_box->data.p_elst->entry_count );

   p_box->data.p_elst->segment_duration =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint64_t) );
   p_box->data.p_elst->media_time =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint64_t) );
   p_box->data.p_elst->media_rate_integer =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint16_t) );
   p_box->data.p_elst->media_rate_fraction =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint16_t) );
   if( p_box->data.p_elst->segment_duration == NULL
      || p_box->data.p_elst->media_time == NULL
      || p_box->data.p_elst->media_rate_integer == NULL
      || p_box->data.p_elst->media_rate_fraction == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }


   for( i = 0; i < p_box->data.p_elst->entry_count; i++ )
   {
      if( p_box->data.p_elst->version == 1 )
      {

         MP4_GET8BYTES( p_box->data.p_elst->segment_duration[i] );

         MP4_GET8BYTES( p_box->data.p_elst->media_time[i] );
      }
      else
      {

         MP4_GET4BYTES( p_box->data.p_elst->segment_duration[i] );

         MP4_GET4BYTES( p_box->data.p_elst->media_time[i] );
         p_box->data.p_elst->media_time[i] = (int32_t)p_box->data.p_elst->media_time[i];
      }

      MP4_GET2BYTES( p_box->data.p_elst->media_rate_integer[i] );
      MP4_GET2BYTES( p_box->data.p_elst->media_rate_fraction[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"elst\" entry-count %lu",
      (unsigned long)p_box->data.p_elst->entry_count );
#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_elstFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_elst_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_elst );


   MP4_GET4BYTES( p_box->data.p_elst->entry_count );

   p_box->data.p_elst->segment_duration =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint64_t) );
   p_box->data.p_elst->media_time =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint64_t) );
   p_box->data.p_elst->media_rate_integer =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint16_t) );
   p_box->data.p_elst->media_rate_fraction =
      calloc( p_box->data.p_elst->entry_count, sizeof(uint16_t) );
   if( p_box->data.p_elst->segment_duration == NULL
      || p_box->data.p_elst->media_time == NULL
      || p_box->data.p_elst->media_rate_integer == NULL
      || p_box->data.p_elst->media_rate_fraction == NULL )
   {
      MP4_READBOX_EXIT( 0 );
   }


   for( i = 0; i < p_box->data.p_elst->entry_count; i++ )
   {
      if( p_box->data.p_elst->version == 1 )
      {

         MP4_GET8BYTES( p_box->data.p_elst->segment_duration[i] );

         MP4_GET8BYTES( p_box->data.p_elst->media_time[i] );
      }
      else
      {

         MP4_GET4BYTES( p_box->data.p_elst->segment_duration[i] );

         MP4_GET4BYTES( p_box->data.p_elst->media_time[i] );
         p_box->data.p_elst->media_time[i] = (int32_t)p_box->data.p_elst->media_time[i];
      }

      MP4_GET2BYTES( p_box->data.p_elst->media_rate_integer[i] );
      MP4_GET2BYTES( p_box->data.p_elst->media_rate_fraction[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"elst\" entry-count %lu",
      (unsigned long)p_box->data.p_elst->entry_count );
#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_cprt( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i_language;
   unsigned int i = 0;

   MP4_READBOX_ENTER( mp4_box_data_cprt_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_cprt );

   i_language = SwapBE16( p_peek );
   for( i = 0; i < 3; i++ )
   {
      p_box->data.p_cprt->language[i] =
         ( ( i_language >> ( (2-i)*5 ) )&0x1f ) + 0x60;
   }
   p_peek += 2; i_read -= 2;
   MP4_GETSTRINGZ( p_box->data.p_cprt->psz_notice );

#ifdef MP4_VERBOSE
    printf( "read box: \"cprt\" language %c%c%c notice %s",
      p_box->data.p_cprt->language[0],
      p_box->data.p_cprt->language[1],
      p_box->data.p_cprt->language[2],
      p_box->data.p_cprt->psz_notice );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_cprtFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i_language;
   unsigned int i = 0;

   MP4_READBOX_ENTERFromBuffer( mp4_box_data_cprt_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_cprt );

   i_language = SwapBE16( p_peek );
   for( i = 0; i < 3; i++ )
   {
      p_box->data.p_cprt->language[i] =
         ( ( i_language >> ( (2-i)*5 ) )&0x1f ) + 0x60;
   }
   p_peek += 2; i_read -= 2;
   MP4_GETSTRINGZ( p_box->data.p_cprt->psz_notice );

#ifdef MP4_VERBOSE
    printf( "read box: \"cprt\" language %c%c%c notice %s",
      p_box->data.p_cprt->language[0],
      p_box->data.p_cprt->language[1],
      p_box->data.p_cprt->language[2],
      p_box->data.p_cprt->psz_notice );
#endif

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_cprt( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_cprt->psz_notice );
}


static int MP4_ReadBox_dcom( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_dcom_t );

   MP4_GETFOURCC( p_box->data.p_dcom->algorithm );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"dcom\" compression algorithm : %4.4s",
      (char*)&p_box->data.p_dcom->algorithm );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_dcomFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_dcom_t );

   MP4_GETFOURCC( p_box->data.p_dcom->algorithm );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"dcom\" compression algorithm : %4.4s",
      (char*)&p_box->data.p_dcom->algorithm );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_cmvd( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_cmvd_t );

   MP4_GET4BYTES( p_box->data.p_cmvd->uncompressed_size );

   p_box->data.p_cmvd->compressed_size = i_read;

   if( !( p_box->data.p_cmvd->data = malloc( i_read ) ) )
      MP4_READBOX_EXIT( 0 );

   /* now copy compressed data */
   memcpy( p_box->data.p_cmvd->data, p_peek,i_read);

   p_box->data.p_cmvd->b_compressed = 1;

#ifdef MP4_VERBOSE
    printf( "read box: \"cmvd\" compressed data size %d",
      p_box->data.p_cmvd->compressed_size );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_cmvdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_cmvd_t );

   MP4_GET4BYTES( p_box->data.p_cmvd->uncompressed_size );

   p_box->data.p_cmvd->compressed_size = i_read;

   if( !( p_box->data.p_cmvd->data = malloc( i_read ) ) )
      MP4_READBOX_EXIT( 0 );

   /* now copy compressed data */
   memcpy( p_box->data.p_cmvd->data, p_peek,i_read);

   p_box->data.p_cmvd->b_compressed = 1;

#ifdef MP4_VERBOSE
    printf( "read box: \"cmvd\" compressed data size %d",
      p_box->data.p_cmvd->compressed_size );
#endif

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_cmvd( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_cmvd->data );
}


static int MP4_ReadBox_cmov( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_t *p_dcom;
   mp4_box_t *p_cmvd;

#ifdef HAVE_ZLIB_H
   stream_t *p_stream_memory;
   z_stream z_data;
   uint8_t *p_data;
   int i_result;
#endif

   if( !( p_box->data.p_cmov = calloc(1, sizeof( mp4_box_data_cmov_t ) ) ) )
      return 0;

   if( !p_box->p_father ||
      ( p_box->p_father->i_type != ATOM_moov &&
      p_box->p_father->i_type != ATOM_foov ) )
   {
      printf( "Read box: \"cmov\" box alone" );
      return 1;
   }

   if( !MP4_ReadBoxContainer( p_stream, p_box ) )
   {
      return 0;
   }

   if( ( p_dcom = MP4_BoxGet( p_box, "dcom" ) ) == NULL ||
      ( p_cmvd = MP4_BoxGet( p_box, "cmvd" ) ) == NULL ||
      p_cmvd->data.p_cmvd->data == NULL )
   {
      printf( "read box: \"cmov\" incomplete" );
      return 0;
   }

   if( p_dcom->data.p_dcom->algorithm != ATOM_zlib )
   {
      printf( "read box: \"cmov\" compression algorithm : %4.4s "
         "not supported", (char*)&p_dcom->data.p_dcom->algorithm );
      return 0;
   }

#ifndef HAVE_ZLIB_H
   printf(  "read box: \"cmov\" zlib unsupported" );
   return 0;
#else

   /* decompress data */
   /* allocate a new buffer */
   if( !( p_data = malloc( p_cmvd->data.p_cmvd->uncompressed_size ) ) )
      return 0;
   /* init default structures */
   z_data.next_in   = p_cmvd->data.p_cmvd->data;
   z_data.avail_in  = p_cmvd->data.p_cmvd->compressed_size;
   z_data.next_out  = p_data;
   z_data.avail_out = p_cmvd->data.p_cmvd->uncompressed_size;
   z_data.zalloc    = (alloc_func)Z_NULL;
   z_data.zfree     = (free_func)Z_NULL;
   z_data.opaque    = (voidpf)Z_NULL;

   /* init zlib */
   if( inflateInit( &z_data ) != Z_OK )
   {
      msg_Err( p_stream, "read box: \"cmov\" error while uncompressing" );
      free( p_data );
      return 0;
   }

   /* uncompress */
   i_result = inflate( &z_data, Z_NO_FLUSH );
   if( i_result != Z_OK && i_result != Z_STREAM_END )
   {
      msg_Err( p_stream, "read box: \"cmov\" error while uncompressing" );
      free( p_data );
      return 0;
   }

   if( p_cmvd->data.p_cmvd->uncompressed_size != z_data.total_out )
   {
      printf( "read box: \"cmov\" uncompressing data size "
         "mismatch" );
   }
   p_cmvd->data.p_cmvd->uncompressed_size = z_data.total_out;

   /* close zlib */
   if( inflateEnd( &z_data ) != Z_OK )
   {
      printf( "read box: \"cmov\" error while uncompressing "
         "data (ignored)" );
   }

   free( p_cmvd->data.p_cmvd->p_data );
   p_cmvd->data.p_cmvd->data = p_data;
   p_cmvd->data.p_cmvd->b_compressed = 0;

    printf( "read box: \"cmov\" box successfully uncompressed" );

   /* now create a memory stream */
   p_stream_memory =
      stream_MemoryNew( VLC_OBJECT(p_stream), p_cmvd->data.p_cmvd->data,
      p_cmvd->data.p_cmvd->uncompressed_size, true );

   /* and read uncompressd moov */
   p_box->data.p_cmov->moov = MP4_ReadBox( p_stream_memory, NULL );

   stream_Delete( p_stream_memory );

#ifdef MP4_VERBOSE
    printf( "read box: \"cmov\" compressed movie header completed");
#endif

   return p_box->data.p_cmov->moov ? 1 : 0;
#endif /* HAVE_ZLIB_H */
}

static int MP4_ReadBox_cmovFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_t *p_dcom;
   mp4_box_t *p_cmvd;

#ifdef HAVE_ZLIB_H
   stream_t *p_stream_memory;
   z_stream z_data;
   uint8_t *p_data;
   int i_result;
#endif

   if( !( p_box->data.p_cmov = calloc(1, sizeof( mp4_box_data_cmov_t ) ) ) )
      return 0;

   if( !p_box->p_father ||
      ( p_box->p_father->i_type != ATOM_moov &&
      p_box->p_father->i_type != ATOM_foov ) )
   {
      printf( "Read box: \"cmov\" box alone" );
      return 1;
   }

   if( !MP4_ReadBoxContainerFromBuffer( p_stream, p_box ) )
   {
      return 0;
   }

   if( ( p_dcom = MP4_BoxGet( p_box, "dcom" ) ) == NULL ||
      ( p_cmvd = MP4_BoxGet( p_box, "cmvd" ) ) == NULL ||
      p_cmvd->data.p_cmvd->data == NULL )
   {
      printf( "read box: \"cmov\" incomplete" );
      return 0;
   }

   if( p_dcom->data.p_dcom->algorithm != ATOM_zlib )
   {
      printf( "read box: \"cmov\" compression algorithm : %4.4s "
         "not supported", (char*)&p_dcom->data.p_dcom->algorithm );
      return 0;
   }

#ifndef HAVE_ZLIB_H
   printf(  "read box: \"cmov\" zlib unsupported" );
   return 0;
#else

   /* decompress data */
   /* allocate a new buffer */
   if( !( p_data = malloc( p_cmvd->data.p_cmvd->uncompressed_size ) ) )
      return 0;
   /* init default structures */
   z_data.next_in   = p_cmvd->data.p_cmvd->data;
   z_data.avail_in  = p_cmvd->data.p_cmvd->compressed_size;
   z_data.next_out  = p_data;
   z_data.avail_out = p_cmvd->data.p_cmvd->uncompressed_size;
   z_data.zalloc    = (alloc_func)Z_NULL;
   z_data.zfree     = (free_func)Z_NULL;
   z_data.opaque    = (voidpf)Z_NULL;

   /* init zlib */
   if( inflateInit( &z_data ) != Z_OK )
   {
      msg_Err( p_stream, "read box: \"cmov\" error while uncompressing" );
      free( p_data );
      return 0;
   }

   /* uncompress */
   i_result = inflate( &z_data, Z_NO_FLUSH );
   if( i_result != Z_OK && i_result != Z_STREAM_END )
   {
      msg_Err( p_stream, "read box: \"cmov\" error while uncompressing" );
      free( p_data );
      return 0;
   }

   if( p_cmvd->data.p_cmvd->uncompressed_size != z_data.total_out )
   {
      printf( "read box: \"cmov\" uncompressing data size "
         "mismatch" );
   }
   p_cmvd->data.p_cmvd->uncompressed_size = z_data.total_out;

   /* close zlib */
   if( inflateEnd( &z_data ) != Z_OK )
   {
      printf( "read box: \"cmov\" error while uncompressing "
         "data (ignored)" );
   }

   free( p_cmvd->data.p_cmvd->p_data );
   p_cmvd->data.p_cmvd->data = p_data;
   p_cmvd->data.p_cmvd->b_compressed = 0;

    printf( "read box: \"cmov\" box successfully uncompressed" );

   /* now create a memory stream */
   p_stream_memory =
      stream_MemoryNew( VLC_OBJECT(p_stream), p_cmvd->data.p_cmvd->data,
      p_cmvd->data.p_cmvd->uncompressed_size, true );

   /* and read uncompressd moov */
   p_box->data.p_cmov->moov = MP4_ReadBox( p_stream_memory, NULL );

   stream_Delete( p_stream_memory );

#ifdef MP4_VERBOSE
    printf( "read box: \"cmov\" compressed movie header completed");
#endif

   return p_box->data.p_cmov->moov ? 1 : 0;
#endif /* HAVE_ZLIB_H */
}

static int MP4_ReadBox_rdrf( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t i_len;
   unsigned i = 0;
   MP4_READBOX_ENTER( mp4_box_data_rdrf_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_rdrf );
   MP4_GETFOURCC( p_box->data.p_rdrf->ref_type );
   MP4_GET4BYTES( i_len );
   i_len++;

   if( i_len > 0 )
   {
      p_box->data.p_rdrf->psz_ref = malloc( i_len );
      if( p_box->data.p_rdrf->psz_ref == NULL )
         MP4_READBOX_EXIT( 0 );
      i_len--;

      for( i = 0; i < i_len; i++ )
      {
         MP4_GET1BYTE( p_box->data.p_rdrf->psz_ref[i] );
      }
      p_box->data.p_rdrf->psz_ref[i_len] = '\0';
   }
   else
   {
      p_box->data.p_rdrf->psz_ref = NULL;
   }

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rdrf\" type:%4.4s ref %s",
      (char*)&p_box->data.p_rdrf->ref_type,
      p_box->data.p_rdrf->psz_ref );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_rdrfFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t i_len;
   unsigned i = 0;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_rdrf_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_rdrf );
   MP4_GETFOURCC( p_box->data.p_rdrf->ref_type );
   MP4_GET4BYTES( i_len );
   i_len++;

   if( i_len > 0 )
   {
      p_box->data.p_rdrf->psz_ref = malloc( i_len );
      if( p_box->data.p_rdrf->psz_ref == NULL )
         MP4_READBOX_EXIT( 0 );
      i_len--;

      for( i = 0; i < i_len; i++ )
      {
         MP4_GET1BYTE( p_box->data.p_rdrf->psz_ref[i] );
      }
      p_box->data.p_rdrf->psz_ref[i_len] = '\0';
   }
   else
   {
      p_box->data.p_rdrf->psz_ref = NULL;
   }

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rdrf\" type:%4.4s ref %s",
      (char*)&p_box->data.p_rdrf->ref_type,
      p_box->data.p_rdrf->psz_ref );
#endif

   MP4_READBOX_EXIT( 1 );
}


static void MP4_FreeBox_rdrf( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_rdrf->psz_ref );
}


static int MP4_ReadBox_rmdr( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_rmdr_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_rmdr );

   MP4_GET4BYTES( p_box->data.p_rmdr->rate );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rmdr\" rate:%d",
      p_box->data.p_rmdr->rate );
#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_rmdrFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_rmdr_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_rmdr );

   MP4_GET4BYTES( p_box->data.p_rmdr->rate );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rmdr\" rate:%d",
      p_box->data.p_rmdr->rate );
#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_rmqu( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_rmqu_t );

   MP4_GET4BYTES( p_box->data.p_rmqu->quality );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rmqu\" quality:%d",
      p_box->data.p_rmqu->quality );
#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_rmquFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_rmqu_t );

   MP4_GET4BYTES( p_box->data.p_rmqu->quality );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rmqu\" quality:%d",
      p_box->data.p_rmqu->quality );
#endif
   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_rmvc( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_rmvc_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_rmvc );

   MP4_GETFOURCC( p_box->data.p_rmvc->gestaltType );
   MP4_GET4BYTES( p_box->data.p_rmvc->val1 );
   MP4_GET4BYTES( p_box->data.p_rmvc->val2 );
   MP4_GET2BYTES( p_box->data.p_rmvc->checkType );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rmvc\" gestaltType:%4.4s val1:0x%x val2:0x%x checkType:0x%x",
      (char*)&p_box->data.p_rmvc->gestaltType,
      p_box->data.p_rmvc->val1,p_box->data.p_rmvc->val2,
      p_box->data.p_rmvc->checkType );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_rmvcFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_rmvc_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_rmvc );

   MP4_GETFOURCC( p_box->data.p_rmvc->gestaltType );
   MP4_GET4BYTES( p_box->data.p_rmvc->val1 );
   MP4_GET4BYTES( p_box->data.p_rmvc->val2 );
   MP4_GET2BYTES( p_box->data.p_rmvc->checkType );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"rmvc\" gestaltType:%4.4s val1:0x%x val2:0x%x checkType:0x%x",
      (char*)&p_box->data.p_rmvc->gestaltType,
      p_box->data.p_rmvc->val1,p_box->data.p_rmvc->val2,
      p_box->data.p_rmvc->checkType );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_frma( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_frma_t );

   MP4_GETFOURCC( p_box->data.p_frma->type );

#ifdef MP4_VERBOSE
    printf( "read box: \"frma\" i_type:%4.4s",
      (char *)&p_box->data.p_frma->i_type );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_frmaFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_frma_t );

   MP4_GETFOURCC( p_box->data.p_frma->type );

#ifdef MP4_VERBOSE
    printf( "read box: \"frma\" i_type:%4.4s",
      (char *)&p_box->data.p_frma->i_type );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_skcr( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_skcr_t );

   MP4_GET4BYTES( p_box->data.p_skcr->init );
   MP4_GET4BYTES( p_box->data.p_skcr->encr );
   MP4_GET4BYTES( p_box->data.p_skcr->decr );

#ifdef MP4_VERBOSE
    printf( "read box: \"skcr\" i_init:%d i_encr:%d i_decr:%d",
      p_box->data.p_skcr->init,
      p_box->data.p_skcr->encr,
      p_box->data.p_skcr->decr );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_skcrFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_skcr_t );

   MP4_GET4BYTES( p_box->data.p_skcr->init );
   MP4_GET4BYTES( p_box->data.p_skcr->encr );
   MP4_GET4BYTES( p_box->data.p_skcr->decr );

#ifdef MP4_VERBOSE
    printf( "read box: \"skcr\" i_init:%d i_encr:%d i_decr:%d",
      p_box->data.p_skcr->init,
      p_box->data.p_skcr->encr,
      p_box->data.p_skcr->decr );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_drms( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_t *p_drms_box = p_box;
   void *p_drms = NULL;

   MP4_READBOX_ENTER( uint8_t );

   do
   {
      p_drms_box = p_drms_box->p_father;
   } while( p_drms_box && p_drms_box->i_type != ATOM_drms
      && p_drms_box->i_type != ATOM_drmi );

   if( p_drms_box && p_drms_box->i_type == ATOM_drms )
      p_drms = p_drms_box->data.p_sample_soun->drms;
   else if( p_drms_box && p_drms_box->i_type == ATOM_drmi )
      p_drms = p_drms_box->data.p_sample_vide->drms;

   if( p_drms_box && p_drms )
   {
      int i_ret = drms_init( p_drms, p_box->i_type, p_peek, i_read );
      if( i_ret )
      {
         assert(0);
         //             const char *psz_error;
         // 
         //             switch( i_ret )
         //             {
         //                 case -1: psz_error = "unimplemented"; break;
         //                 case -2: psz_error = "invalid argument"; break;
         //                 case -3: psz_error = "could not get system key"; break;
         //                 case -4: psz_error = "could not get SCI data"; break;
         //                 case -5: psz_error = "no user key found in SCI data"; break;
         //                 case -6: psz_error = "invalid user key"; break;
         //                 default: psz_error = "unknown error"; break;
         //             }
         //             if MP4_BOX_TYPE_ASCII()
         //                 msg_Err( p_stream, "drms_init(%4.4s) failed (%s)",
         //                         (char *)&p_box->i_type, psz_error );
         //             else
         //                 msg_Err( p_stream, "drms_init(c%3.3s) failed (%s)",
         //                         (char *)&p_box->i_type+1, psz_error );
         // 
         //             drms_free( p_drms );
         // 
         //             if( p_drms_box->i_type == ATOM_drms )
         //                 p_drms_box->data.p_sample_soun->p_drms = NULL;
         //             else if( p_drms_box->i_type == ATOM_drmi )
         //                 p_drms_box->data.p_sample_vide->p_drms = NULL;
      }
   }

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_drmsFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_t *p_drms_box = p_box;
   void *p_drms = NULL;

   MP4_READBOX_ENTERFromBuffer( uint8_t );

   do
   {
      p_drms_box = p_drms_box->p_father;
   } while( p_drms_box && p_drms_box->i_type != ATOM_drms
      && p_drms_box->i_type != ATOM_drmi );

   if( p_drms_box && p_drms_box->i_type == ATOM_drms )
      p_drms = p_drms_box->data.p_sample_soun->drms;
   else if( p_drms_box && p_drms_box->i_type == ATOM_drmi )
      p_drms = p_drms_box->data.p_sample_vide->drms;

   if( p_drms_box && p_drms )
   {
      int i_ret = drms_init( p_drms, p_box->i_type, p_peek, i_read );
      if( i_ret )
      {
         assert(0);
         //             const char *psz_error;
         // 
         //             switch( i_ret )
         //             {
         //                 case -1: psz_error = "unimplemented"; break;
         //                 case -2: psz_error = "invalid argument"; break;
         //                 case -3: psz_error = "could not get system key"; break;
         //                 case -4: psz_error = "could not get SCI data"; break;
         //                 case -5: psz_error = "no user key found in SCI data"; break;
         //                 case -6: psz_error = "invalid user key"; break;
         //                 default: psz_error = "unknown error"; break;
         //             }
         //             if MP4_BOX_TYPE_ASCII()
         //                 msg_Err( p_stream, "drms_init(%4.4s) failed (%s)",
         //                         (char *)&p_box->i_type, psz_error );
         //             else
         //                 msg_Err( p_stream, "drms_init(c%3.3s) failed (%s)",
         //                         (char *)&p_box->i_type+1, psz_error );
         // 
         //             drms_free( p_drms );
         // 
         //             if( p_drms_box->i_type == ATOM_drms )
         //                 p_drms_box->data.p_sample_soun->p_drms = NULL;
         //             else if( p_drms_box->i_type == ATOM_drmi )
         //                 p_drms_box->data.p_sample_vide->p_drms = NULL;
      }
   }

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_name( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_name_t );

   p_box->data.p_name->psz_text = malloc( p_box->i_size + 1 - 8 ); /* +\0, -name, -size */
   if( p_box->data.p_name->psz_text == NULL )
      MP4_READBOX_EXIT( 0 );

   memcpy( p_box->data.p_name->psz_text, p_peek, p_box->i_size - 8 );
   p_box->data.p_name->psz_text[p_box->i_size - 8] = '\0';

#ifdef MP4_VERBOSE
    printf( "read box: \"name\" text=`%s'",
      p_box->data.p_name->psz_text );
#endif
   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_nameFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_name_t );

   p_box->data.p_name->psz_text = malloc( p_box->i_size + 1 - 8 ); /* +\0, -name, -size */
   if( p_box->data.p_name->psz_text == NULL )
      MP4_READBOX_EXIT( 0 );

   memcpy( p_box->data.p_name->psz_text, p_peek, p_box->i_size - 8 );
   p_box->data.p_name->psz_text[p_box->i_size - 8] = '\0';

#ifdef MP4_VERBOSE
    printf( "read box: \"name\" text=`%s'",
      p_box->data.p_name->psz_text );
#endif
   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_name( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_name->psz_text );
}

static int MP4_ReadBox_0xa9xxx( stream_t *p_stream, mp4_box_t *p_box )
{
   uint16_t i16;

   MP4_READBOX_ENTER( mp4_box_data_0xa9xxx_t );

   p_box->data.p_0xa9xxx->psz_text = NULL;

   MP4_GET2BYTES( i16 );

   if( i16 > 0 )
   {
      int i_length = i16;

      MP4_GET2BYTES( i16 );
      if( i_length >= i_read ) i_length = i_read + 1;

      p_box->data.p_0xa9xxx->psz_text = malloc( i_length );
      if( p_box->data.p_0xa9xxx->psz_text == NULL )
         MP4_READBOX_EXIT( 0 );

      i_length--;
      memcpy( p_box->data.p_0xa9xxx->psz_text,
         p_peek, i_length );
      p_box->data.p_0xa9xxx->psz_text[i_length] = '\0';

#ifdef MP4_VERBOSE
       printf(
         "read box: \"c%3.3s\" text=`%s'",
         ((char*)&p_box->i_type + 1),
         p_box->data.p_0xa9xxx->psz_text );
#endif
   }
   else
   {
      uint32_t i_data_len;
      uint32_t i_data_tag;
      /* try iTune/Quicktime format, rewind to start */
      p_peek -= 2; i_read += 2;
      // we are expecting a 'data' box

      MP4_GET4BYTES( i_data_len );
      if( i_data_len > i_read ) i_data_len = i_read;
      MP4_GETFOURCC( i_data_tag );
      if( (i_data_len > 0) && (i_data_tag == ATOM_data) )
      {
         /* data box contains a version/flags field */
         uint32_t i_version;
         uint32_t i_reserved;
         MP4_GET4BYTES( i_version );
         MP4_GET4BYTES( i_reserved );
         // version should be 0, flags should be 1 for text, 0 for data
         if( ( i_version == 0x00000001 ) && (i_data_len >= 12 ) )
         {
            // the rest is the text
            i_data_len -= 12;
            p_box->data.p_0xa9xxx->psz_text = malloc( i_data_len + 1 );
            if( p_box->data.p_0xa9xxx->psz_text == NULL )
               MP4_READBOX_EXIT( 0 );

            memcpy( p_box->data.p_0xa9xxx->psz_text,
               p_peek, i_data_len );
            p_box->data.p_0xa9xxx->psz_text[i_data_len] = '\0';
#ifdef MP4_VERBOSE
             printf(
               "read box: \"c%3.3s\" text=`%s'",
               ((char*)&p_box->i_type+1),
               p_box->data.p_0xa9xxx->psz_text );
#endif
         }
         else
         {
            // TODO: handle data values for ID3 tag values, track num or cover art,etc...
         }
      }
   }

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_0xa9xxxFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   uint16_t i16;

   MP4_READBOX_ENTERFromBuffer( mp4_box_data_0xa9xxx_t );

   p_box->data.p_0xa9xxx->psz_text = NULL;

   MP4_GET2BYTES( i16 );

   if( i16 > 0 )
   {
      int i_length = i16;

      MP4_GET2BYTES( i16 );
      if( i_length >= i_read ) i_length = i_read + 1;

      p_box->data.p_0xa9xxx->psz_text = malloc( i_length );
      if( p_box->data.p_0xa9xxx->psz_text == NULL )
         MP4_READBOX_EXIT( 0 );

      i_length--;
      memcpy( p_box->data.p_0xa9xxx->psz_text,
         p_peek, i_length );
      p_box->data.p_0xa9xxx->psz_text[i_length] = '\0';

#ifdef MP4_VERBOSE
       printf(
         "read box: \"c%3.3s\" text=`%s'",
         ((char*)&p_box->i_type + 1),
         p_box->data.p_0xa9xxx->psz_text );
#endif
   }
   else
   {
      uint32_t i_data_len;
      uint32_t i_data_tag;
      /* try iTune/Quicktime format, rewind to start */
      p_peek -= 2; i_read += 2;
      // we are expecting a 'data' box

      MP4_GET4BYTES( i_data_len );
      if( i_data_len > i_read ) i_data_len = i_read;
      MP4_GETFOURCC( i_data_tag );
      if( (i_data_len > 0) && (i_data_tag == ATOM_data) )
      {
         /* data box contains a version/flags field */
         uint32_t i_version;
         uint32_t i_reserved;
         MP4_GET4BYTES( i_version );
         MP4_GET4BYTES( i_reserved );
         // version should be 0, flags should be 1 for text, 0 for data
         if( ( i_version == 0x00000001 ) && (i_data_len >= 12 ) )
         {
            // the rest is the text
            i_data_len -= 12;
            p_box->data.p_0xa9xxx->psz_text = malloc( i_data_len + 1 );
            if( p_box->data.p_0xa9xxx->psz_text == NULL )
               MP4_READBOX_EXIT( 0 );

            memcpy( p_box->data.p_0xa9xxx->psz_text,
               p_peek, i_data_len );
            p_box->data.p_0xa9xxx->psz_text[i_data_len] = '\0';
#ifdef MP4_VERBOSE
             printf(
               "read box: \"c%3.3s\" text=`%s'",
               ((char*)&p_box->i_type+1),
               p_box->data.p_0xa9xxx->psz_text );
#endif
         }
         else
         {
            // TODO: handle data values for ID3 tag values, track num or cover art,etc...
         }
      }
   }

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_0xa9xxx( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_0xa9xxx->psz_text );
}

/* Chapter support */
static void MP4_FreeBox_chpl( mp4_box_t *p_box )
{
   mp4_box_data_chpl_t *p_chpl = p_box->data.p_chpl;
   unsigned int i;
   for(  i = 0; i < p_chpl->chapter; i++ )
      free( p_chpl->p_chapter[i].psz_name );
}

static int MP4_ReadBox_chpl( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_data_chpl_t *p_chpl;
   uint32_t i_dummy;
   int i;
   MP4_READBOX_ENTER( mp4_box_data_chpl_t );

   p_chpl = p_box->data.p_chpl;

   MP4_GETVERSIONFLAGS( p_chpl );

   MP4_GET4BYTES( i_dummy );

   MP4_GET1BYTE( p_chpl->chapter );

   for( i = 0; i < p_chpl->chapter; i++ )
   {
      uint64_t i_start;
      uint8_t i_len;
      int i_copy;
      MP4_GET8BYTES( i_start );
      MP4_GET1BYTE( i_len );

      p_chpl->p_chapter[i].psz_name = malloc( i_len + 1 );
      if( !p_chpl->p_chapter[i].psz_name )
         MP4_READBOX_EXIT( 0 );

      i_copy = min( i_len, i_read );
      if( i_copy > 0 )
         memcpy( p_chpl->p_chapter[i].psz_name, p_peek, i_copy );
      p_chpl->p_chapter[i].psz_name[i_copy] = '\0';
      p_chpl->p_chapter[i].start = i_start;

      p_peek += i_copy;
      i_read -= i_copy;
   }
   /* Bubble sort by increasing start date */
   do
   {
      for( i = 0; i < p_chpl->chapter - 1; i++ )
      {
         if( p_chpl->p_chapter[i].start > p_chpl->p_chapter[i+1].start )
         {
            char *psz = p_chpl->p_chapter[i+1].psz_name;
            int64_t i64 = p_chpl->p_chapter[i+1].start;

            p_chpl->p_chapter[i+1].psz_name = p_chpl->p_chapter[i].psz_name;
            p_chpl->p_chapter[i+1].start = p_chpl->p_chapter[i].start;

            p_chpl->p_chapter[i].psz_name = psz;
            p_chpl->p_chapter[i].start = i64;

            i = -1;
            break;
         }
      }
   } while( i == -1 );

#ifdef MP4_VERBOSE
    printf( "read box: \"chpl\" %d chapters",
      p_chpl->chapter );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_chplFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   mp4_box_data_chpl_t *p_chpl;
   uint32_t i_dummy;
   int i;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_chpl_t );

   p_chpl = p_box->data.p_chpl;

   MP4_GETVERSIONFLAGS( p_chpl );

   MP4_GET4BYTES( i_dummy );

   MP4_GET1BYTE( p_chpl->chapter );

   for( i = 0; i < p_chpl->chapter; i++ )
   {
      uint64_t i_start;
      uint8_t i_len;
      int i_copy;
      MP4_GET8BYTES( i_start );
      MP4_GET1BYTE( i_len );

      p_chpl->p_chapter[i].psz_name = malloc( i_len + 1 );
      if( !p_chpl->p_chapter[i].psz_name )
         MP4_READBOX_EXIT( 0 );

      i_copy = min( i_len, i_read );
      if( i_copy > 0 )
         memcpy( p_chpl->p_chapter[i].psz_name, p_peek, i_copy );
      p_chpl->p_chapter[i].psz_name[i_copy] = '\0';
      p_chpl->p_chapter[i].start = i_start;

      p_peek += i_copy;
      i_read -= i_copy;
   }
   /* Bubble sort by increasing start date */
   do
   {
      for( i = 0; i < p_chpl->chapter - 1; i++ )
      {
         if( p_chpl->p_chapter[i].start > p_chpl->p_chapter[i+1].start )
         {
            char *psz = p_chpl->p_chapter[i+1].psz_name;
            int64_t i64 = p_chpl->p_chapter[i+1].start;

            p_chpl->p_chapter[i+1].psz_name = p_chpl->p_chapter[i].psz_name;
            p_chpl->p_chapter[i+1].start = p_chpl->p_chapter[i].start;

            p_chpl->p_chapter[i].psz_name = psz;
            p_chpl->p_chapter[i].start = i64;

            i = -1;
            break;
         }
      }
   } while( i == -1 );

#ifdef MP4_VERBOSE
    printf( "read box: \"chpl\" %d chapters",
      p_chpl->chapter );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_tref_generic( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i;
   MP4_READBOX_ENTER( mp4_box_data_tref_generic_t );

   p_box->data.p_tref_generic->track_ID = NULL;
   p_box->data.p_tref_generic->entry_count = i_read / sizeof(uint32_t);
   if( p_box->data.p_tref_generic->entry_count > 0 )
      p_box->data.p_tref_generic->track_ID = calloc( p_box->data.p_tref_generic->entry_count, sizeof(uint32_t) );
   if( p_box->data.p_tref_generic->track_ID == NULL )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < p_box->data.p_tref_generic->entry_count; i++ )
   {
      MP4_GET4BYTES( p_box->data.p_tref_generic->track_ID[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"chap\" %d references",
      p_box->data.p_tref_generic->entry_count );
#endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_tref_genericFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   unsigned int i;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_tref_generic_t );

   p_box->data.p_tref_generic->track_ID = NULL;
   p_box->data.p_tref_generic->entry_count = i_read / sizeof(uint32_t);
   if( p_box->data.p_tref_generic->entry_count > 0 )
      p_box->data.p_tref_generic->track_ID = calloc( p_box->data.p_tref_generic->entry_count, sizeof(uint32_t) );
   if( p_box->data.p_tref_generic->track_ID == NULL )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < p_box->data.p_tref_generic->entry_count; i++ )
   {
      MP4_GET4BYTES( p_box->data.p_tref_generic->track_ID[i] );
   }

#ifdef MP4_VERBOSE
    printf( "read box: \"chap\" %d references",
      p_box->data.p_tref_generic->entry_count );
#endif

   MP4_READBOX_EXIT( 1 );
}


static void MP4_FreeBox_tref_generic( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_tref_generic->track_ID );
}

static int MP4_ReadBox_meta( stream_t *p_stream, mp4_box_t *p_box )
{
   uint8_t meta_data[8];
   int i_actually_read;

   // skip over box header
   i_actually_read = stream_read( p_stream, meta_data, 8 );
   if( i_actually_read < 8 )
      return 0;

   /* meta content starts with a 4 byte version/flags value (should be 0) */
   i_actually_read = stream_read( p_stream, meta_data, 4 );
   if( i_actually_read < 4 )
      return 0;

   /* then it behaves like a container */
   return MP4_ReadBoxContainerRaw( p_stream, p_box );
}

static int MP4_ReadBox_metaFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   uint8_t meta_data[8];
   int i_actually_read;

   // skip over box header
   i_actually_read = buffer_read( p_stream, meta_data, 8 );
   if( i_actually_read < 8 )
      return 0;

   /* meta content starts with a 4 byte version/flags value (should be 0) */
   i_actually_read = buffer_read( p_stream, meta_data, 4 );
   if( i_actually_read < 4 )
      return 0;

   /* then it behaves like a container */
   return MP4_ReadBoxContainerRawFromBuffer( p_stream, p_box );
}

static int MP4_ReadBox_iods( stream_t *p_stream, mp4_box_t *p_box )
{
   char i_unused;

   MP4_READBOX_ENTER( mp4_box_data_iods_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_iods );

   MP4_GET1BYTE( i_unused ); /* tag */
   MP4_GET1BYTE( i_unused ); /* length */

   MP4_GET2BYTES( p_box->data.p_iods->object_descriptor ); /* 10bits, 6 other bits
                                                           are used for other flags */
   MP4_GET1BYTE( p_box->data.p_iods->OD_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->scene_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->audio_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->visual_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->graphics_profile_level );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"iods\" objectDescriptorId: %i, OD: %i, scene: %i, audio: %i, "
      "visual: %i, graphics: %i",
      p_box->data.p_iods->object_descriptor >> 6,
      p_box->data.p_iods->OD_profile_level,
      p_box->data.p_iods->scene_profile_level,
      p_box->data.p_iods->audio_profile_level,
      p_box->data.p_iods->visual_profile_level,
      p_box->data.p_iods->graphics_profile_level );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_iodsFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   char i_unused;

   MP4_READBOX_ENTERFromBuffer( mp4_box_data_iods_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_iods );

   MP4_GET1BYTE( i_unused ); /* tag */
   MP4_GET1BYTE( i_unused ); /* length */

   MP4_GET2BYTES( p_box->data.p_iods->object_descriptor ); /* 10bits, 6 other bits
                                                           are used for other flags */
   MP4_GET1BYTE( p_box->data.p_iods->OD_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->scene_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->audio_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->visual_profile_level );
   MP4_GET1BYTE( p_box->data.p_iods->graphics_profile_level );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"iods\" objectDescriptorId: %i, OD: %i, scene: %i, audio: %i, "
      "visual: %i, graphics: %i",
      p_box->data.p_iods->object_descriptor >> 6,
      p_box->data.p_iods->OD_profile_level,
      p_box->data.p_iods->scene_profile_level,
      p_box->data.p_iods->audio_profile_level,
      p_box->data.p_iods->visual_profile_level,
      p_box->data.p_iods->graphics_profile_level );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_pasp( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_pasp_t );

   MP4_GET4BYTES( p_box->data.p_pasp->horizontal_spacing );
   MP4_GET4BYTES( p_box->data.p_pasp->vertical_spacing );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"paps\" %dx%d",
      p_box->data.p_pasp->horizontal_spacing,
      p_box->data.p_pasp->vertical_spacing);
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_paspFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_pasp_t );

   MP4_GET4BYTES( p_box->data.p_pasp->horizontal_spacing );
   MP4_GET4BYTES( p_box->data.p_pasp->vertical_spacing );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"paps\" %dx%d",
      p_box->data.p_pasp->horizontal_spacing,
      p_box->data.p_pasp->vertical_spacing);
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mehd( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_mehd_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_mehd );
   if( p_box->data.p_mehd->version == 1 )
      MP4_GET8BYTES( p_box->data.p_mehd->fragment_duration );
   else /* version == 0 */
      MP4_GET4BYTES( p_box->data.p_mehd->fragment_duration );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"mehd\" frag dur. %"PRIu64"",
      p_box->data.p_mehd->fragment_duration );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mehdFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_mehd_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_mehd );
   if( p_box->data.p_mehd->version == 1 )
      MP4_GET8BYTES( p_box->data.p_mehd->fragment_duration );
   else /* version == 0 */
      MP4_GET4BYTES( p_box->data.p_mehd->fragment_duration );

#ifdef MP4_VERBOSE
    printf(
      "read box: \"mehd\" frag dur. %"PRIu64"",
      p_box->data.p_mehd->fragment_duration );
#endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_trex( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_trex_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_trex );

   MP4_GET4BYTES( p_box->data.p_trex->track_ID );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_description_index );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_duration );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_size );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_flags );

   // #ifdef MP4_VERBOSE
   //      printf(
   //              "read box: \"trex\" trackID: %"PRIu32"",
   //              p_box->data.p_trex->track_ID );
   // #endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_trexFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_trex_t );
   MP4_GETVERSIONFLAGS( p_box->data.p_trex );

   MP4_GET4BYTES( p_box->data.p_trex->track_ID );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_description_index );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_duration );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_size );
   MP4_GET4BYTES( p_box->data.p_trex->default_sample_flags );

   // #ifdef MP4_VERBOSE
   //      printf(
   //              "read box: \"trex\" trackID: %"PRIu32"",
   //              p_box->data.p_trex->track_ID );
   // #endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_sdtp( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t i_sample_count;
   uint32_t i;
   mp4_box_data_sdtp_t *p_sdtp = NULL;
   MP4_READBOX_ENTER( mp4_box_data_sdtp_t );
   p_sdtp = p_box->data.p_sdtp;
   MP4_GETVERSIONFLAGS( p_box->data.p_sdtp );
   i_sample_count = i_read;

   p_sdtp->sample_table = calloc( i_sample_count, 1 );

   if( !p_sdtp->sample_table )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < i_sample_count; i++ )
      MP4_GET1BYTE( p_sdtp->sample_table[i] );

   // #ifdef MP4_VERBOSE
   //     msg_Info( p_stream, "i_sample_count is %"PRIu32"", i_sample_count );
   //      printf(
   //              "read box: \"sdtp\" head: %"PRIx8" %"PRIx8" %"PRIx8" %"PRIx8"",
   //                  p_sdtp->sample_table[0],
   //                  p_sdtp->sample_table[1],
   //                  p_sdtp->sample_table[2],
   //                  p_sdtp->sample_table[3] );
   // #endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_sdtpFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t i_sample_count;
   uint32_t i;
   mp4_box_data_sdtp_t *p_sdtp = NULL;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_sdtp_t );
   p_sdtp = p_box->data.p_sdtp;
   MP4_GETVERSIONFLAGS( p_box->data.p_sdtp );
   i_sample_count = i_read;

   p_sdtp->sample_table = calloc( i_sample_count, 1 );

   if( !p_sdtp->sample_table )
      MP4_READBOX_EXIT( 0 );

   for( i = 0; i < i_sample_count; i++ )
      MP4_GET1BYTE( p_sdtp->sample_table[i] );

   // #ifdef MP4_VERBOSE
   //     msg_Info( p_stream, "i_sample_count is %"PRIu32"", i_sample_count );
   //      printf(
   //              "read box: \"sdtp\" head: %"PRIx8" %"PRIx8" %"PRIx8" %"PRIx8"",
   //                  p_sdtp->sample_table[0],
   //                  p_sdtp->sample_table[1],
   //                  p_sdtp->sample_table[2],
   //                  p_sdtp->sample_table[3] );
   // #endif

   MP4_READBOX_EXIT( 1 );
}

static void MP4_FreeBox_sdtp( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_sdtp->sample_table );
}

static int MP4_ReadBox_mfro( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTER( mp4_box_data_mfro_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_mfro );
   MP4_GET4BYTES( p_box->data.p_mfro->size );

   // #ifdef MP4_VERBOSE
   //      printf(
   //              "read box: \"mfro\" size: %"PRIu32"",
   //              p_box->data.p_mfro->i_size);
   // #endif

   MP4_READBOX_EXIT( 1 );
}

static int MP4_ReadBox_mfroFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_mfro_t );

   MP4_GETVERSIONFLAGS( p_box->data.p_mfro );
   MP4_GET4BYTES( p_box->data.p_mfro->size );

   // #ifdef MP4_VERBOSE
   //      printf(
   //              "read box: \"mfro\" size: %"PRIu32"",
   //              p_box->data.p_mfro->i_size);
   // #endif

   MP4_READBOX_EXIT( 1 );
}


static int MP4_ReadBox_tfra( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t i_number_of_entries;
   mp4_box_data_tfra_t *p_tfra;
   uint32_t i_lengths;
   size_t size;
   uint32_t i;
   MP4_READBOX_ENTER( mp4_box_data_tfra_t );
   p_tfra = p_box->data.p_tfra;
   MP4_GETVERSIONFLAGS( p_box->data.p_tfra );

   MP4_GET4BYTES( p_tfra->track_ID );
   i_lengths = 0;
   MP4_GET4BYTES( i_lengths );
   MP4_GET4BYTES( p_tfra->number_of_entries );
   i_number_of_entries = p_tfra->number_of_entries;
   p_tfra->length_size_of_traf_num = i_lengths >> 4;
   p_tfra->length_size_of_trun_num = ( i_lengths & 0x0c ) >> 2;
   p_tfra->length_size_of_sample_num = i_lengths & 0x03;

   size = 4 + 4*p_tfra->version; /* size in {4, 8} */
   p_tfra->time = calloc( i_number_of_entries, size );
   p_tfra->moof_offset = calloc( i_number_of_entries, size );

   size = 1 + p_tfra->length_size_of_traf_num; /* size in [|1, 4|] */
   p_tfra->traf_number = calloc( i_number_of_entries, size );
   size = 1 + p_tfra->length_size_of_trun_num;
   p_tfra->trun_number = calloc( i_number_of_entries, size );
   size = 1 + p_tfra->length_size_of_sample_num;
   p_tfra->sample_number = calloc( i_number_of_entries, size );

   if( !p_tfra->time || !p_tfra->moof_offset || !p_tfra->traf_number
      || !p_tfra->trun_number || !p_tfra->sample_number )
      goto error;

   for( i = 0; i < i_number_of_entries; i++ )
   {
      if( p_tfra->version == 1 )
      {
         MP4_GET8BYTES( p_tfra->time[i*2] );
         MP4_GET8BYTES( p_tfra->moof_offset[i*2] );
      }
      else
      {
         MP4_GET4BYTES( p_tfra->time[i] );
         MP4_GET4BYTES( p_tfra->moof_offset[i] );
      }
      switch (p_tfra->length_size_of_traf_num)
      {
      case 0:
         MP4_GET1BYTE( p_tfra->traf_number[i] );
         break;
      case 1:
         MP4_GET2BYTES( p_tfra->traf_number[i*2] );
         break;
      case 2:
         MP4_GET3BYTES( p_tfra->traf_number[i*3] );
         break;
      case 3:
         MP4_GET4BYTES( p_tfra->traf_number[i*4] );
         break;
      default:
         goto error;
      }

      switch (p_tfra->length_size_of_trun_num)
      {
      case 0:
         MP4_GET1BYTE( p_tfra->trun_number[i] );
         break;
      case 1:
         MP4_GET2BYTES( p_tfra->trun_number[i*2] );
         break;
      case 2:
         MP4_GET3BYTES( p_tfra->trun_number[i*3] );
         break;
      case 3:
         MP4_GET4BYTES( p_tfra->trun_number[i*4] );
         break;
      default:
         goto error;
      }

      switch (p_tfra->length_size_of_sample_num)
      {
      case 0:
         MP4_GET1BYTE( p_tfra->sample_number[i] );
         break;
      case 1:
         MP4_GET2BYTES( p_tfra->sample_number[i*2] );
         break;
      case 2:
         MP4_GET3BYTES( p_tfra->sample_number[i*3] );
         break;
      case 3:
         MP4_GET4BYTES( p_tfra->sample_number[i*4] );
         break;
      default:
         goto error;
      }
   }

   // #ifdef MP4_VERBOSE
   //     if( p_tfra->version == 0 )
   //     {
   //          printf( "time[0]: %"PRIu32", moof_offset[0]: %"PRIx32"",
   //                          p_tfra->time[0], p_tfra->moof_offset[0] );
   // 
   //          printf( "time[1]: %"PRIu32", moof_offset[1]: %"PRIx32"",
   //                          p_tfra->time[1], p_tfra->moof_offset[1] );
   //     }
   //     else
   //     {
   //          printf( "time[0]: %"PRIu64", moof_offset[0]: %"PRIx64"",
   //                 ((uint64_t *)(p_tfra->time))[0],
   //                 ((uint64_t *)(p_tfra->moof_offset))[0] );
   // 
   //          printf( "time[1]: %"PRIu64", moof_offset[1]: %"PRIx64"",
   //                 ((uint64_t *)(p_tfra->time))[1],
   //                 ((uint64_t *)(p_tfra->moof_offset))[1] );
   //     }
   // 
   //     msg_Info( p_stream, "number_of_entries is %"PRIu32"", number_of_entries );
   //     msg_Info( p_stream, "track ID is: %"PRIu32"", p_tfra->track_ID );
   // #endif

   MP4_READBOX_EXIT( 1 );
error:
   MP4_READBOX_EXIT( 0 );
}

static int MP4_ReadBox_tfraFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   uint32_t i_number_of_entries;
   mp4_box_data_tfra_t *p_tfra;
   uint32_t i_lengths;
   size_t size;
   uint32_t i;
   MP4_READBOX_ENTERFromBuffer( mp4_box_data_tfra_t );
   p_tfra = p_box->data.p_tfra;
   MP4_GETVERSIONFLAGS( p_box->data.p_tfra );

   MP4_GET4BYTES( p_tfra->track_ID );
   i_lengths = 0;
   MP4_GET4BYTES( i_lengths );
   MP4_GET4BYTES( p_tfra->number_of_entries );
   i_number_of_entries = p_tfra->number_of_entries;
   p_tfra->length_size_of_traf_num = i_lengths >> 4;
   p_tfra->length_size_of_trun_num = ( i_lengths & 0x0c ) >> 2;
   p_tfra->length_size_of_sample_num = i_lengths & 0x03;

   size = 4 + 4*p_tfra->version; /* size in {4, 8} */
   p_tfra->time = calloc( i_number_of_entries, size );
   p_tfra->moof_offset = calloc( i_number_of_entries, size );

   size = 1 + p_tfra->length_size_of_traf_num; /* size in [|1, 4|] */
   p_tfra->traf_number = calloc( i_number_of_entries, size );
   size = 1 + p_tfra->length_size_of_trun_num;
   p_tfra->trun_number = calloc( i_number_of_entries, size );
   size = 1 + p_tfra->length_size_of_sample_num;
   p_tfra->sample_number = calloc( i_number_of_entries, size );

   if( !p_tfra->time || !p_tfra->moof_offset || !p_tfra->traf_number
      || !p_tfra->trun_number || !p_tfra->sample_number )
      goto error;

   for( i = 0; i < i_number_of_entries; i++ )
   {
      if( p_tfra->version == 1 )
      {
         MP4_GET8BYTES( p_tfra->time[i*2] );
         MP4_GET8BYTES( p_tfra->moof_offset[i*2] );
      }
      else
      {
         MP4_GET4BYTES( p_tfra->time[i] );
         MP4_GET4BYTES( p_tfra->moof_offset[i] );
      }
      switch (p_tfra->length_size_of_traf_num)
      {
      case 0:
         MP4_GET1BYTE( p_tfra->traf_number[i] );
         break;
      case 1:
         MP4_GET2BYTES( p_tfra->traf_number[i*2] );
         break;
      case 2:
         MP4_GET3BYTES( p_tfra->traf_number[i*3] );
         break;
      case 3:
         MP4_GET4BYTES( p_tfra->traf_number[i*4] );
         break;
      default:
         goto error;
      }

      switch (p_tfra->length_size_of_trun_num)
      {
      case 0:
         MP4_GET1BYTE( p_tfra->trun_number[i] );
         break;
      case 1:
         MP4_GET2BYTES( p_tfra->trun_number[i*2] );
         break;
      case 2:
         MP4_GET3BYTES( p_tfra->trun_number[i*3] );
         break;
      case 3:
         MP4_GET4BYTES( p_tfra->trun_number[i*4] );
         break;
      default:
         goto error;
      }

      switch (p_tfra->length_size_of_sample_num)
      {
      case 0:
         MP4_GET1BYTE( p_tfra->sample_number[i] );
         break;
      case 1:
         MP4_GET2BYTES( p_tfra->sample_number[i*2] );
         break;
      case 2:
         MP4_GET3BYTES( p_tfra->sample_number[i*3] );
         break;
      case 3:
         MP4_GET4BYTES( p_tfra->sample_number[i*4] );
         break;
      default:
         goto error;
      }
   }

   // #ifdef MP4_VERBOSE
   //     if( p_tfra->version == 0 )
   //     {
   //          printf( "time[0]: %"PRIu32", moof_offset[0]: %"PRIx32"",
   //                          p_tfra->time[0], p_tfra->moof_offset[0] );
   // 
   //          printf( "time[1]: %"PRIu32", moof_offset[1]: %"PRIx32"",
   //                          p_tfra->time[1], p_tfra->moof_offset[1] );
   //     }
   //     else
   //     {
   //          printf( "time[0]: %"PRIu64", moof_offset[0]: %"PRIx64"",
   //                 ((uint64_t *)(p_tfra->time))[0],
   //                 ((uint64_t *)(p_tfra->moof_offset))[0] );
   // 
   //          printf( "time[1]: %"PRIu64", moof_offset[1]: %"PRIx64"",
   //                 ((uint64_t *)(p_tfra->time))[1],
   //                 ((uint64_t *)(p_tfra->moof_offset))[1] );
   //     }
   // 
   //     msg_Info( p_stream, "number_of_entries is %"PRIu32"", number_of_entries );
   //     msg_Info( p_stream, "track ID is: %"PRIu32"", p_tfra->track_ID );
   // #endif

   MP4_READBOX_EXIT( 1 );
error:
   MP4_READBOX_EXIT( 0 );
}

static void MP4_FreeBox_tfra( mp4_box_t *p_box )
{
   FREENULL( p_box->data.p_tfra->time );
   FREENULL( p_box->data.p_tfra->moof_offset );
   FREENULL( p_box->data.p_tfra->traf_number );
   FREENULL( p_box->data.p_tfra->trun_number );
   FREENULL( p_box->data.p_tfra->sample_number );
}


/* For generic */
static int MP4_ReadBox_default( stream_t *p_stream, mp4_box_t *p_box )
{
   if( !p_box->p_father )
   {
      goto unknown;
   }
   if( p_box->p_father->i_type == ATOM_stsd )
   {
      mp4_box_t *p_mdia = MP4_BoxGet( p_box, "../../../.." );
      mp4_box_t *p_hdlr;

      if( p_mdia == NULL || p_mdia->i_type != ATOM_mdia ||
         (p_hdlr = MP4_BoxGet( p_mdia, "hdlr" )) == NULL )
      {
         goto unknown;
      }
      switch( p_hdlr->data.p_hdlr->handler_type )
      {
      case ATOM_soun:
         return MP4_ReadBox_sample_soun( p_stream, p_box );
      case ATOM_vide:
         return MP4_ReadBox_sample_vide( p_stream, p_box );
      case ATOM_text:
         return MP4_ReadBox_sample_text( p_stream, p_box );
      case ATOM_mmth:
         return MP4_ReadBox_sample_mmth( p_stream, p_box );
      case ATOM_tx3g:
      case ATOM_sbtl:
         return MP4_ReadBox_sample_tx3g( p_stream, p_box );
      default:
    	  printf(
            "unknown handler type in stsd (incompletely loaded1)" );
         return 1;
      }
   }

unknown:
   if MP4_BOX_TYPE_ASCII()
      printf(
      "unknown box type %4.4s (incompletely loaded2)",
      (char*)&p_box->i_type );
   else
	   printf(
      "unknown box type c%3.3s (incompletely loaded3)",
      (char*)&p_box->i_type+1 );

   return 1;
}

static int MP4_ReadBox_defaultFromBuffer( stream_t *p_stream, mp4_box_t *p_box )
{
   if( !p_box->p_father )
   {
      goto unknown;
   }
   if( p_box->p_father->i_type == ATOM_stsd )
   {
      mp4_box_t *p_mdia = MP4_BoxGet( p_box, "../../../.." );
      mp4_box_t *p_hdlr;

      if( p_mdia == NULL || p_mdia->i_type != ATOM_mdia ||
         (p_hdlr = MP4_BoxGet( p_mdia, "hdlr" )) == NULL )
      {
         goto unknown;
      }
      switch( p_hdlr->data.p_hdlr->handler_type )
      {
      case ATOM_soun:
         return MP4_ReadBox_sample_sounFromBuffer( p_stream, p_box );
      case ATOM_vide:
         return MP4_ReadBox_sample_videFromBuffer( p_stream, p_box );
      case ATOM_text:
         return MP4_ReadBox_sample_textFromBuffer( p_stream, p_box );
      case ATOM_mmth:
         return MP4_ReadBox_sample_mmthFromBuffer( p_stream, p_box );
      case ATOM_tx3g:
      case ATOM_sbtl:
         return MP4_ReadBox_sample_tx3gFromBuffer( p_stream, p_box );
      default:
    	  printf(
            "unknown handler type in stsd (incompletely loaded1)" );
         return 1;
      }
   }

unknown:
   if MP4_BOX_TYPE_ASCII()
      printf(
      "unknown box type %4.4s (incompletely loaded2)",
      (char*)&p_box->i_type );
   else
	   printf(
      "unknown box type c%3.3s (incompletely loaded3)",
      (char*)&p_box->i_type+1 );

   return 1;
}

static const struct
{
   uint32_t i_type;
   int  (*MP4_ReadBox_function )( stream_t *p_stream, mp4_box_t *p_box );
   void (*MP4_FreeBox_function )( mp4_box_t *p_box );
} MP4_Box_Function [] =
{
   /* Containers */
   { ATOM_moov,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_trak,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_mdia,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_moof,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_minf,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_stbl,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_dinf,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_edts,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_udta,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_nmhd,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_hnti,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_rmra,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_rmda,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_tref,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_gmhd,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_wave,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_ilst,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_mvex,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },

   /* specific box */
   { ATOM_ftyp,    MP4_ReadBox_ftyp,         MP4_FreeBox_ftyp },
   { ATOM_mmpu,    MP4_ReadBox_mmpu,         MP4_FreeBox_mmpu },
   { ATOM_tfdt,    MP4_ReadBox_tfdt,         MP4_FreeBox_Common },
   { ATOM_cmov,    MP4_ReadBox_cmov,         MP4_FreeBox_Common },
   { ATOM_mvhd,    MP4_ReadBox_mvhd,         MP4_FreeBox_Common },
   { ATOM_tkhd,    MP4_ReadBox_tkhd,         MP4_FreeBox_Common },
   { ATOM_hint,    MP4_ReadBox_hint,         MP4_FreeBox_Common },
   { ATOM_mdhd,    MP4_ReadBox_mdhd,         MP4_FreeBox_Common },
   { ATOM_hdlr,    MP4_ReadBox_hdlr,         MP4_FreeBox_hdlr },
   { ATOM_vmhd,    MP4_ReadBox_vmhd,         MP4_FreeBox_Common },
   { ATOM_smhd,    MP4_ReadBox_smhd,         MP4_FreeBox_Common },
   { ATOM_hmhd,    MP4_ReadBox_hmhd,         MP4_FreeBox_Common },
   { ATOM_url,     MP4_ReadBox_url,          MP4_FreeBox_url },
   { ATOM_urn,     MP4_ReadBox_urn,          MP4_FreeBox_urn },
   { ATOM_dref,    MP4_ReadBox_dref,         MP4_FreeBox_Common },
   { ATOM_stts,    MP4_ReadBox_stts,         MP4_FreeBox_stts },
   { ATOM_ctts,    MP4_ReadBox_ctts,         MP4_FreeBox_ctts },
   { ATOM_stsd,    MP4_ReadBox_stsd,         MP4_FreeBox_Common },
   { ATOM_stsz,    MP4_ReadBox_stsz,         MP4_FreeBox_stsz },
   { ATOM_stsc,    MP4_ReadBox_stsc,         MP4_FreeBox_stsc },
   { ATOM_stco,    MP4_ReadBox_stco_co64,    MP4_FreeBox_stco_co64 },
   { ATOM_co64,    MP4_ReadBox_stco_co64,    MP4_FreeBox_stco_co64 },
   { ATOM_stss,    MP4_ReadBox_stss,         MP4_FreeBox_stss },
   { ATOM_stsh,    MP4_ReadBox_stsh,         MP4_FreeBox_stsh },
   { ATOM_stdp,    MP4_ReadBox_stdp,         MP4_FreeBox_stdp },
   { ATOM_padb,    MP4_ReadBox_padb,         MP4_FreeBox_padb },
   { ATOM_elst,    MP4_ReadBox_elst,         MP4_FreeBox_elst },
   { ATOM_cprt,    MP4_ReadBox_cprt,         MP4_FreeBox_cprt },
   { ATOM_esds,    MP4_ReadBox_esds,         MP4_FreeBox_esds },
   { ATOM_dcom,    MP4_ReadBox_dcom,         MP4_FreeBox_Common },
   { ATOM_cmvd,    MP4_ReadBox_cmvd,         MP4_FreeBox_cmvd },
   { ATOM_avcC,    MP4_ReadBox_avcC,         MP4_FreeBox_avcC },
   { ATOM_dac3,    MP4_ReadBox_dac3,         MP4_FreeBox_Common },
   { ATOM_enda,    MP4_ReadBox_enda,         MP4_FreeBox_Common },
   { ATOM_gnre,    MP4_ReadBox_gnre,         MP4_FreeBox_Common },
   { ATOM_trkn,    MP4_ReadBox_trkn,         MP4_FreeBox_Common },
   { ATOM_iods,    MP4_ReadBox_iods,         MP4_FreeBox_Common },
   { ATOM_pasp,    MP4_ReadBox_pasp,         MP4_FreeBox_Common },

   /* Nothing to do with this box */
   { ATOM_mdat,    MP4_ReadBoxSkip,          MP4_FreeBox_Common },
   { ATOM_skip,    MP4_ReadBoxSkip,          MP4_FreeBox_Common },
   { ATOM_free,    MP4_ReadBoxSkip,          MP4_FreeBox_Common },
   { ATOM_wide,    MP4_ReadBoxSkip,          MP4_FreeBox_Common },

   /* for codecs */
   { ATOM_soun,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_ms02,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_ms11,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_ms55,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM__mp3,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_mp4a,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_twos,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_sowt,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_QDMC,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_QDM2,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_ima4,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_IMA4,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_dvi,     MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_alaw,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_ulaw,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_raw,     MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_MAC3,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_MAC6,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_Qclp,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_samr,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_sawb,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_OggS,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_alac,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },

   { ATOM_drmi,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_vide,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_mp4v,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_SVQ1,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_SVQ3,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_ZyGo,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_DIVX,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_XVID,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_h263,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_s263,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_cvid,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3IV1,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3iv1,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3IV2,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3iv2,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3IVD,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3ivd,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3VID,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_3vid,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_mjpa,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_mjpb,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_qdrw,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_mp2v,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_hdv2,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },

   { ATOM_mjqt,    MP4_ReadBox_default,      NULL }, /* found in mjpa/b */
   { ATOM_mjht,    MP4_ReadBox_default,      NULL },

   { ATOM_dvc,     MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_dvp,     MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_dv5n,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_dv5p,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_VP31,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_vp31,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_h264,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },

   { ATOM_jpeg,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_avc1,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },

   { ATOM_yv12,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },
   { ATOM_yuv2,    MP4_ReadBox_sample_vide,  MP4_FreeBox_sample_vide },

   { ATOM_mp4s,    MP4_ReadBox_sample_mp4s,  MP4_FreeBox_Common },

   /* XXX there is 2 box where we could find this entry stbl and tref*/
   { ATOM_hint,    MP4_ReadBox_default,      MP4_FreeBox_Common },
   { ATOM_mmth,    MP4_ReadBox_sample_mmth,  MP4_FreeBox_Common },

   /* found in tref box */
   { ATOM_dpnd,    MP4_ReadBox_default,      NULL },
   { ATOM_ipir,    MP4_ReadBox_default,      NULL },
   { ATOM_mpod,    MP4_ReadBox_default,      NULL },
   { ATOM_chap,    MP4_ReadBox_tref_generic, MP4_FreeBox_tref_generic },

   /* found in hnti */
   { ATOM_rtp,     MP4_ReadBox_default,      NULL },

   /* found in rmra */
   { ATOM_rdrf,    MP4_ReadBox_rdrf,         MP4_FreeBox_rdrf   },
   { ATOM_rmdr,    MP4_ReadBox_rmdr,         MP4_FreeBox_Common },
   { ATOM_rmqu,    MP4_ReadBox_rmqu,         MP4_FreeBox_Common },
   { ATOM_rmvc,    MP4_ReadBox_rmvc,         MP4_FreeBox_Common },

   { ATOM_drms,    MP4_ReadBox_sample_soun,  MP4_FreeBox_sample_soun },
   { ATOM_sinf,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_schi,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_user,    MP4_ReadBox_drms,         MP4_FreeBox_Common },
   { ATOM_key,     MP4_ReadBox_drms,         MP4_FreeBox_Common },
   { ATOM_iviv,    MP4_ReadBox_drms,         MP4_FreeBox_Common },
   { ATOM_priv,    MP4_ReadBox_drms,         MP4_FreeBox_Common },
   { ATOM_frma,    MP4_ReadBox_frma,         MP4_FreeBox_Common },
   { ATOM_skcr,    MP4_ReadBox_skcr,         MP4_FreeBox_Common },

   /* found in udta */
   { ATOM_0xa9nam, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9aut, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9cpy, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9swr, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9inf, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9ART, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9dir, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9cmt, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9req, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9day, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9des, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9fmt, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9prd, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9prf, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9src, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9alb, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9dis, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9enc, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9gen, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9trk, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9dsa, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9hst, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9url, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9ope, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9com, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9wrt, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9too, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9wrn, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9mak, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9mod, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9PRD, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9grp, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9lyr, MP4_ReadBox_0xa9xxx,      MP4_FreeBox_0xa9xxx },

   { ATOM_chpl,    MP4_ReadBox_chpl,         MP4_FreeBox_chpl },

   /* iTunes/Quicktime meta info */
   { ATOM_meta,    MP4_ReadBox_meta,         MP4_FreeBox_Common },
   { ATOM_name,    MP4_ReadBox_name,         MP4_FreeBox_name },

   /* found in smoothstreaming */
   { ATOM_traf,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_mfra,    MP4_ReadBoxContainer,     MP4_FreeBox_Common },
   { ATOM_mfhd,    MP4_ReadBox_mfhd,         MP4_FreeBox_Common },
   { ATOM_tfhd,    MP4_ReadBox_tfhd,         MP4_FreeBox_Common },
   { ATOM_trun,    MP4_ReadBox_trun,         MP4_FreeBox_trun },
   { ATOM_trex,    MP4_ReadBox_trex,         MP4_FreeBox_Common },
   { ATOM_mehd,    MP4_ReadBox_mehd,         MP4_FreeBox_Common },
   { ATOM_sdtp,    MP4_ReadBox_sdtp,         MP4_FreeBox_sdtp },
   { ATOM_tfra,    MP4_ReadBox_tfra,         MP4_FreeBox_tfra },
   { ATOM_mfro,    MP4_ReadBox_mfro,         MP4_FreeBox_Common },

   /* Last entry */
   { 0,              MP4_ReadBox_default,      NULL }
};

static const struct
{
   uint32_t i_type;
   int  (*MP4_ReadBox_function )( stream_t *p_stream, mp4_box_t *p_box );
   void (*MP4_FreeBox_function )( mp4_box_t *p_box );
} MP4_Box_FunctionFromBuffer [] =
{
   /* Containers */
   { ATOM_moov,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_trak,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_mdia,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_moof,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_minf,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_stbl,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_dinf,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_edts,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_udta,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_nmhd,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_hnti,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_rmra,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_rmda,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_tref,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_gmhd,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_wave,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_ilst,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_mvex,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },

   /* specific box */
   { ATOM_ftyp,    MP4_ReadBox_ftypFromBuffer,         MP4_FreeBox_ftyp },
   { ATOM_mmpu,    MP4_ReadBox_mmpuFromBuffer,         MP4_FreeBox_mmpu },
   { ATOM_tfdt,    MP4_ReadBox_tfdtFromBuffer,         MP4_FreeBox_Common },
   { ATOM_cmov,    MP4_ReadBox_cmovFromBuffer,         MP4_FreeBox_Common },
   { ATOM_mvhd,    MP4_ReadBox_mvhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_tkhd,    MP4_ReadBox_tkhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_hint,    MP4_ReadBox_hintFromBuffer,         MP4_FreeBox_Common },
   { ATOM_mdhd,    MP4_ReadBox_mdhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_hdlr,    MP4_ReadBox_hdlrFromBuffer,         MP4_FreeBox_hdlr },
   { ATOM_vmhd,    MP4_ReadBox_vmhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_smhd,    MP4_ReadBox_smhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_hmhd,    MP4_ReadBox_hmhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_url,     MP4_ReadBox_urlFromBuffer,          MP4_FreeBox_url },
   { ATOM_urn,     MP4_ReadBox_urnFromBuffer,          MP4_FreeBox_urn },
   { ATOM_dref,    MP4_ReadBox_drefFromBuffer,         MP4_FreeBox_Common },
   { ATOM_stts,    MP4_ReadBox_sttsFromBuffer,         MP4_FreeBox_stts },
   { ATOM_ctts,    MP4_ReadBox_cttsFromBuffer,         MP4_FreeBox_ctts },
   { ATOM_stsd,    MP4_ReadBox_stsdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_stsz,    MP4_ReadBox_stszFromBuffer,         MP4_FreeBox_stsz },
   { ATOM_stsc,    MP4_ReadBox_stscFromBuffer,         MP4_FreeBox_stsc },
   { ATOM_stco,    MP4_ReadBox_stco_co64FromBuffer,    MP4_FreeBox_stco_co64 },
   { ATOM_co64,    MP4_ReadBox_stco_co64FromBuffer,    MP4_FreeBox_stco_co64 },
   { ATOM_stss,    MP4_ReadBox_stssFromBuffer,         MP4_FreeBox_stss },
   { ATOM_stsh,    MP4_ReadBox_stshFromBuffer,         MP4_FreeBox_stsh },
   { ATOM_stdp,    MP4_ReadBox_stdpFromBuffer,         MP4_FreeBox_stdp },
   { ATOM_padb,    MP4_ReadBox_padbFromBuffer,         MP4_FreeBox_padb },
   { ATOM_elst,    MP4_ReadBox_elstFromBuffer,         MP4_FreeBox_elst },
   { ATOM_cprt,    MP4_ReadBox_cprtFromBuffer,         MP4_FreeBox_cprt },
   { ATOM_esds,    MP4_ReadBox_esdsFromBuffer,         MP4_FreeBox_esds },
   { ATOM_dcom,    MP4_ReadBox_dcomFromBuffer,         MP4_FreeBox_Common },
   { ATOM_cmvd,    MP4_ReadBox_cmvdFromBuffer,         MP4_FreeBox_cmvd },
   { ATOM_avcC,    MP4_ReadBox_avcCFromBuffer,         MP4_FreeBox_avcC },
   { ATOM_dac3,    MP4_ReadBox_dac3FromBuffer,         MP4_FreeBox_Common },
   { ATOM_enda,    MP4_ReadBox_endaFromBuffer,         MP4_FreeBox_Common },
   { ATOM_gnre,    MP4_ReadBox_gnreFromBuffer,         MP4_FreeBox_Common },
   { ATOM_trkn,    MP4_ReadBox_trknFromBuffer,         MP4_FreeBox_Common },
   { ATOM_iods,    MP4_ReadBox_iodsFromBuffer,         MP4_FreeBox_Common },
   { ATOM_pasp,    MP4_ReadBox_paspFromBuffer,         MP4_FreeBox_Common },

   /* Nothing to do with this box */
   { ATOM_mdat,    MP4_ReadBoxSkipFromBuffer,          MP4_FreeBox_Common },
   { ATOM_skip,    MP4_ReadBoxSkipFromBuffer,          MP4_FreeBox_Common },
   { ATOM_free,    MP4_ReadBoxSkipFromBuffer,          MP4_FreeBox_Common },
   { ATOM_wide,    MP4_ReadBoxSkipFromBuffer,          MP4_FreeBox_Common },

   /* for codecs */
   { ATOM_soun,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_ms02,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_ms11,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_ms55,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM__mp3,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_mp4a,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_twos,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_sowt,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_QDMC,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_QDM2,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_ima4,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_IMA4,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_dvi,     MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_alaw,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_ulaw,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_raw,     MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_MAC3,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_MAC6,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_Qclp,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_samr,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_sawb,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_OggS,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_alac,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },

   { ATOM_drmi,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_vide,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_mp4v,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_SVQ1,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_SVQ3,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_ZyGo,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_DIVX,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_XVID,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_h263,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_s263,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_cvid,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3IV1,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3iv1,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3IV2,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3iv2,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3IVD,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3ivd,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3VID,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_3vid,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_mjpa,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_mjpb,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_qdrw,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_mp2v,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_hdv2,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },

   { ATOM_mjqt,    MP4_ReadBox_defaultFromBuffer,      NULL }, /* found in mjpa/b */
   { ATOM_mjht,    MP4_ReadBox_defaultFromBuffer,      NULL },

   { ATOM_dvc,     MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_dvp,     MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_dv5n,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_dv5p,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_VP31,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_vp31,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_h264,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },

   { ATOM_jpeg,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_avc1,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },

   { ATOM_yv12,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },
   { ATOM_yuv2,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_sample_vide },

   { ATOM_mp4s,    MP4_ReadBox_sample_videFromBuffer,  MP4_FreeBox_Common },

   /* XXX there is 2 box where we could find this entry stbl and tref*/
   { ATOM_hint,    MP4_ReadBox_defaultFromBuffer,      MP4_FreeBox_Common },
   { ATOM_mmth,    MP4_ReadBox_sample_mmthFromBuffer,  MP4_FreeBox_Common },

   /* found in tref box */
   { ATOM_dpnd,    MP4_ReadBox_defaultFromBuffer,      NULL },
   { ATOM_ipir,    MP4_ReadBox_defaultFromBuffer,      NULL },
   { ATOM_mpod,    MP4_ReadBox_defaultFromBuffer,      NULL },
   { ATOM_chap,    MP4_ReadBox_tref_genericFromBuffer, MP4_FreeBox_tref_generic },

   /* found in hnti */
   { ATOM_rtp,     MP4_ReadBox_defaultFromBuffer,      NULL },

   /* found in rmra */
   { ATOM_rdrf,    MP4_ReadBox_rdrfFromBuffer,         MP4_FreeBox_rdrf   },
   { ATOM_rmdr,    MP4_ReadBox_rmdrFromBuffer,         MP4_FreeBox_Common },
   { ATOM_rmqu,    MP4_ReadBox_rmquFromBuffer,         MP4_FreeBox_Common },
   { ATOM_rmvc,    MP4_ReadBox_rmvcFromBuffer,         MP4_FreeBox_Common },

   { ATOM_drms,    MP4_ReadBox_sample_sounFromBuffer,  MP4_FreeBox_sample_soun },
   { ATOM_sinf,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_schi,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_user,    MP4_ReadBox_drmsFromBuffer,         MP4_FreeBox_Common },
   { ATOM_key,     MP4_ReadBox_drmsFromBuffer,         MP4_FreeBox_Common },
   { ATOM_iviv,    MP4_ReadBox_drmsFromBuffer,         MP4_FreeBox_Common },
   { ATOM_priv,    MP4_ReadBox_drmsFromBuffer,         MP4_FreeBox_Common },
   { ATOM_frma,    MP4_ReadBox_frmaFromBuffer,         MP4_FreeBox_Common },
   { ATOM_skcr,    MP4_ReadBox_skcrFromBuffer,         MP4_FreeBox_Common },

   /* found in udta */
   { ATOM_0xa9nam, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9aut, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9cpy, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9swr, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9inf, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9ART, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9dir, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9cmt, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9req, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9day, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9des, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9fmt, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9prd, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9prf, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9src, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9alb, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9dis, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9enc, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9gen, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9trk, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9dsa, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9hst, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9url, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9ope, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9com, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9wrt, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9too, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9wrn, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9mak, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9mod, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9PRD, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9grp, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },
   { ATOM_0xa9lyr, MP4_ReadBox_0xa9xxxFromBuffer,      MP4_FreeBox_0xa9xxx },

   { ATOM_chpl,    MP4_ReadBox_chplFromBuffer,         MP4_FreeBox_chpl },

   /* iTunes/Quicktime meta info */
   { ATOM_meta,    MP4_ReadBox_metaFromBuffer,         MP4_FreeBox_Common },
   { ATOM_name,    MP4_ReadBox_nameFromBuffer,         MP4_FreeBox_name },

   /* found in smoothstreaming */
   { ATOM_traf,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_mfra,    MP4_ReadBoxContainerFromBuffer,     MP4_FreeBox_Common },
   { ATOM_mfhd,    MP4_ReadBox_mfhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_tfhd,    MP4_ReadBox_tfhdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_trun,    MP4_ReadBox_trunFromBuffer,         MP4_FreeBox_trun },
   { ATOM_trex,    MP4_ReadBox_trexFromBuffer,         MP4_FreeBox_Common },
   { ATOM_mehd,    MP4_ReadBox_mehdFromBuffer,         MP4_FreeBox_Common },
   { ATOM_sdtp,    MP4_ReadBox_sdtpFromBuffer,         MP4_FreeBox_sdtp },
   { ATOM_tfra,    MP4_ReadBox_tfraFromBuffer,         MP4_FreeBox_tfra },
   { ATOM_mfro,    MP4_ReadBox_mfroFromBuffer,         MP4_FreeBox_Common },

   /* Last entry */
   { 0,              MP4_ReadBox_defaultFromBuffer,      NULL }
};
//get full struct of the mp4,main function
mp4_box_t *MP4_BoxGetRoot(stream_t * s)
{
   mp4_box_t *p_root;
   stream_t *p_stream;
   int i_result;

   p_root = malloc( sizeof( mp4_box_t ) );
   if( p_root == NULL )
      return NULL;

   p_root->i_pos = 0;
   p_root->i_type = ATOM_root;
   p_root->i_shortsize = 1;

   stream_seek(s, 0, SEEK_END);
   p_root->i_size = stream_tell(s);
   stream_seek(s, 0, SEEK_SET);

   CreateUUID( &p_root->i_uuid, p_root->i_type );

   //printf("uuid%s\ntype%d\nsize%d\n",&p_root->i_uuid, p_root->i_type,p_root->i_size);
   p_root->data.p_data = NULL;
   p_root->p_father    = NULL;
   p_root->p_first     = NULL;
   p_root->p_last      = NULL;
   p_root->p_next      = NULL;

   p_stream = s;

   i_result = MP4_ReadBoxContainerRaw( p_stream, p_root );

   if( i_result )
   {
      mp4_box_t *p_moov;
      mp4_box_t *p_cmov;

      /* check if there is a cmov, if so replace
      compressed moov by  uncompressed one */
      if( ( ( p_moov = MP4_BoxGet( p_root, "moov" ) ) &&
         ( p_cmov = MP4_BoxGet( p_root, "moov/cmov" ) ) ) ||
         ( ( p_moov = MP4_BoxGet( p_root, "foov" ) ) &&
         ( p_cmov = MP4_BoxGet( p_root, "foov/cmov" ) ) ) )
      {
         /* rename the compressed moov as a box to skip */
         p_moov->i_type = ATOM_skip;

         /* get uncompressed p_moov */
         p_moov = p_cmov->data.p_cmov->moov;
         p_cmov->data.p_cmov->moov = NULL;

         /* make p_root father of this new moov */
         p_moov->p_father = p_root;

         /* insert this new moov box as first child of p_root */
         p_moov->p_next = p_root->p_first;
         p_root->p_first = p_moov;
      }
   }

   return p_root;
}

mp4_box_t *MP4_BoxGetRootFromBuffer(stream_t * s, unsigned long filesize)
{
   mp4_box_t *p_root;
   stream_t *p_stream;
   int i_result;

   p_root = malloc( sizeof( mp4_box_t ) );
   if( p_root == NULL )
      return NULL;

   p_root->i_pos = 0;
   p_root->i_type = ATOM_root;
   p_root->i_shortsize = 1;

   p_root->i_size = filesize;

   CreateUUID( &p_root->i_uuid, p_root->i_type );

   //printf("uuid%s\ntype%d\nsize%d\n",&p_root->i_uuid, p_root->i_type,p_root->i_size);
   p_root->data.p_data = NULL;
   p_root->p_father    = NULL;
   p_root->p_first     = NULL;
   p_root->p_last      = NULL;
   p_root->p_next      = NULL;

   p_stream = s;

   i_result = MP4_ReadBoxContainerRawFromBuffer( p_stream, p_root );

   if( i_result )
   {
      mp4_box_t *p_moov;
      mp4_box_t *p_cmov;

      /* check if there is a cmov, if so replace
      compressed moov by  uncompressed one */
      if( ( ( p_moov = MP4_BoxGet( p_root, "moov" ) ) &&
         ( p_cmov = MP4_BoxGet( p_root, "moov/cmov" ) ) ) ||
         ( ( p_moov = MP4_BoxGet( p_root, "foov" ) ) &&
         ( p_cmov = MP4_BoxGet( p_root, "foov/cmov" ) ) ) )
      {
         /* rename the compressed moov as a box to skip */
         p_moov->i_type = ATOM_skip;

         /* get uncompressed p_moov */
         p_moov = p_cmov->data.p_cmov->moov;
         p_cmov->data.p_cmov->moov = NULL;

         /* make p_root father of this new moov */
         p_moov->p_father = p_root;

         /* insert this new moov box as first child of p_root */
         p_moov->p_next = p_root->p_first;
         p_root->p_first = p_moov;
      }
   }

   return p_root;
}
//after  get full struct of the mp4,you can use this function to get the special box
mp4_box_t * MP4_BoxSearchBox(mp4_box_t *p_head, uint32_t i_type)
{
	//mp4_box_t *cur = calloc( 1, sizeof( mp4_box_t ) ); /* Needed to ensure simple on error handler */
	mp4_box_t *cur = p_head;

    while (cur != NULL)
    {
		if (cur->i_type==ATOM_moov||cur->i_type==ATOM_trak||cur->i_type==ATOM_mdia||cur->i_type==ATOM_moof \
			||cur->i_type==ATOM_minf||cur->i_type==ATOM_stbl||cur->i_type==ATOM_dinf||cur->i_type==ATOM_edts \
			||cur->i_type==ATOM_udta||cur->i_type==ATOM_nmhd||cur->i_type==ATOM_hnti||cur->i_type==ATOM_rmra \
			||cur->i_type==ATOM_rmda||cur->i_type==ATOM_tref||cur->i_type==ATOM_gmhd||cur->i_type==ATOM_wave \
			||cur->i_type==ATOM_ilst||cur->i_type==ATOM_mvex||cur->i_type==ATOM_stsd||cur->i_type==ATOM_tref \
			||cur->i_type==ATOM_traf||cur->i_type==ATOM_mfra||cur->i_type==ATOM_dref||cur->i_type==ATOM_root)
        {
			printf("current box is %c%c%c%c\n",cur->i_type&0x000000ff,(cur->i_type&0x0000ff00)>>8,(cur->i_type&0x00ff0000)>>16,(cur->i_type&0xff000000)>>24);
			if(cur->i_type==i_type)
				return cur;
        	 // find predecessor
			else {
				cur = cur->p_first;
				return MP4_BoxSearchBox(cur, i_type);
			}
        }
        else
        {
			printf("current box is %c%c%c%c\n",cur->i_type&0x000000ff,(cur->i_type&0x0000ff00)>>8,(cur->i_type&0x00ff0000)>>16,(cur->i_type&0xff000000)>>24);
			if(cur->i_type == i_type)
				{
					return(cur); /*Find the box*/
				}
            else 
				if(cur->p_next)
					cur = cur->p_next;
				else if(cur->p_father->p_next)
					cur = cur->p_father->p_next;
                                else {
                                printf("%s:%d\n", __func__, __LINE__);
				if(!cur->p_father->p_father) {
					printf("Format not supported!\n");
					break;
                                } else if(cur->p_father->p_father->p_next)
					cur = cur->p_father->p_father->p_next;
				else if(cur->p_father->p_father->p_father->p_next)
					cur = cur->p_father->p_father->p_father->p_next;
				else if(cur->p_father->p_father->p_father->p_father->p_next)
					cur = cur->p_father->p_father->p_father->p_father->p_next;
				else if(cur->p_father->p_father->p_father->p_father->p_father->p_next)
					cur = cur->p_father->p_father->p_father->p_father->p_father->p_next;
				else if(cur->p_father->p_father->p_father->p_father->p_father->p_father->p_next)
					cur = cur->p_father->p_father->p_father->p_father->p_father->p_father->p_next;
				else {
					printf("Format not supported!\n");
					return NULL;
				}
				}

        }
    }
    return(NULL);
}

/*****************************************************************************
* MP4_ReadBox : parse the actual box and the children
*  XXX : Do not go to the next box
*****************************************************************************/
static mp4_box_t *MP4_ReadBox( stream_t *p_stream, mp4_box_t *p_father )
{
   mp4_box_t *p_box = calloc( 1, sizeof( mp4_box_t ) ); /* Needed to ensure simple on error handler */
   unsigned int i_index;

   if( p_box == NULL )
      return NULL;

   if( !MP4_ReadBoxCommon( p_stream, p_box ) )
   {
      printf( "cannot read one box" );
      free( p_box );
      return NULL;
   }
   if( !p_box->i_size )
   {
       printf( "found an empty box (null size)" );
      free( p_box );
      return NULL;
   }
   p_box->p_father = p_father;

   /* Now search function to call */
   for( i_index = 0; ; i_index++ )
   {
      if( ( MP4_Box_Function[i_index].i_type == p_box->i_type )||
         ( MP4_Box_Function[i_index].i_type == 0 ) )
      {
         break;
      }
   }

   if( !(MP4_Box_Function[i_index].MP4_ReadBox_function)( p_stream, p_box ) )
   {
      MP4_BoxFree( p_box );
      return NULL;
   }

   return p_box;
}

static mp4_box_t *MP4_ReadBoxFromBuffer( stream_t *p_stream, mp4_box_t *p_father )
{
   mp4_box_t *p_box = calloc( 1, sizeof( mp4_box_t ) ); /* Needed to ensure simple on error handler */
   unsigned int i_index;

   if( p_box == NULL )
      return NULL;

   if( !MP4_ReadBoxCommonFromBuffer( p_stream, p_box ) )
   {
      printf( "cannot read one box" );
      free( p_box );
      return NULL;
   }
   if( !p_box->i_size )
   {
       printf( "found an empty box (null size)" );
      free( p_box );
      return NULL;
   }
   p_box->p_father = p_father;

   /* Now search function to call */
   for( i_index = 0; ; i_index++ )
   {
      if( ( MP4_Box_FunctionFromBuffer[i_index].i_type == p_box->i_type )||
         ( MP4_Box_FunctionFromBuffer[i_index].i_type == 0 ) )
      {
         break;
      }
   }

   if( !(MP4_Box_FunctionFromBuffer[i_index].MP4_ReadBox_function)( p_stream, p_box ) )
   {
      MP4_BoxFreeFromBuffer( p_box );
      return NULL;
   }

   return p_box;
}

void MP4_BoxFree( mp4_box_t *p_box )
{
   unsigned int i_index;
   mp4_box_t    *p_child;

   if( !p_box )
      return; /* hehe */

   for( p_child = p_box->p_first; p_child != NULL; )
   {
      mp4_box_t *p_next;

      p_next = p_child->p_next;
      MP4_BoxFree( p_child );
      p_child = p_next;
   }

   /* Now search function to call */
   if( p_box->data.p_data )
   {
      for( i_index = 0; ; i_index++ )
      {
         if( ( MP4_Box_Function[i_index].i_type == p_box->i_type )||
            ( MP4_Box_Function[i_index].i_type == 0 ) )
         {
            break;
         }
      }
      if( MP4_Box_Function[i_index].MP4_FreeBox_function == NULL )
      {
         /* Should not happen */
         if MP4_BOX_TYPE_ASCII()
		printf(
            "cannot free box %4.4s, type unknown",
            (char*)&p_box->i_type );
         else
        	 printf(
            "cannot free box c%3.3s, type unknown",
            (char*)&p_box->i_type+1 );
      }
      else
      {
         MP4_Box_Function[i_index].MP4_FreeBox_function( p_box );
      }
      free( p_box->data.p_data );
   }
   free( p_box );
}

void MP4_BoxFreeFromBuffer( mp4_box_t *p_box )
{
   unsigned int i_index;
   mp4_box_t    *p_child;

   if( !p_box )
      return; /* hehe */

   for( p_child = p_box->p_first; p_child != NULL; )
   {
      mp4_box_t *p_next;

      p_next = p_child->p_next;
      MP4_BoxFreeFromBuffer( p_child );
      p_child = p_next;
   }

   /* Now search function to call */
   if( p_box->data.p_data )
   {
      for( i_index = 0; ; i_index++ )
      {
         if( ( MP4_Box_FunctionFromBuffer[i_index].i_type == p_box->i_type )||
            ( MP4_Box_FunctionFromBuffer[i_index].i_type == 0 ) )
         {
            break;
         }
      }
      if( MP4_Box_FunctionFromBuffer[i_index].MP4_FreeBox_function == NULL )
      {
         /* Should not happen */
         if MP4_BOX_TYPE_ASCII()
		printf(
            "cannot free box %4.4s, type unknown",
            (char*)&p_box->i_type );
         else
        	 printf(
            "cannot free box c%3.3s, type unknown",
            (char*)&p_box->i_type+1 );
      }
      else
      {
         MP4_Box_FunctionFromBuffer[i_index].MP4_FreeBox_function( p_box );
      }
      free( p_box->data.p_data );
   }
   free( p_box );
}
/*****************************************************************************
*****************************************************************************
**
**  High level methods to acces an MP4 file
**
*****************************************************************************
*****************************************************************************/
static void get_token( char **ppsz_path, char **ppsz_token, int *pi_number )
{
   size_t i_len ;
   if( !*ppsz_path[0] )
   {
      *ppsz_token = NULL;
      *pi_number = 0;
      return;
   }
   i_len = strcspn( *ppsz_path, "/[" );
   if( !i_len && **ppsz_path == '/' )
   {
      i_len = 1;
   }
   *ppsz_token = malloc( i_len + 1 );

   memcpy( *ppsz_token, *ppsz_path, i_len );

   (*ppsz_token)[i_len] = '\0';

   *ppsz_path += i_len;

   if( **ppsz_path == '[' )
   {
      (*ppsz_path)++;
      *pi_number = strtol( *ppsz_path, NULL, 10 );
      while( **ppsz_path && **ppsz_path != ']' )
      {
         (*ppsz_path)++;
      }
      if( **ppsz_path == ']' )
      {
         (*ppsz_path)++;
      }
   }
   else
   {
      *pi_number = 0;
   }
   while( **ppsz_path == '/' )
   {
      (*ppsz_path)++;
   }
}

static void MP4_BoxGet_Internal(mp4_box_t **pp_result,
   mp4_box_t *p_box, const char *psz_fmt, va_list args)
{
   char *psz_dup;
   char *psz_path = malloc(4096);
   char *psz_token;

   if( !p_box )
   {
      *pp_result = NULL;
      return;
   }

   if( vsprintf( psz_path, psz_fmt, args ) == -1 )
      psz_path = NULL;

   if( !psz_path || !psz_path[0] )
   {
      free( psz_path );
      *pp_result = NULL;
      return;
   }

   //    fprintf( stderr, "path:'%s'\n", psz_path );
   psz_dup = psz_path; /* keep this pointer, as it need to be unallocated */
   for( ; ; )
   {
      int i_number;

      get_token( &psz_path, &psz_token, &i_number );
      //        fprintf( stderr, "path:'%s', token:'%s' n:%d\n",
      //                 psz_path,psz_token,i_number );
      if( !psz_token )
      {
         free( psz_dup );
         *pp_result = p_box;
         return;
      }
      else
         if( !strcmp( psz_token, "/" ) )
         {
            /* Find root box */
            while( p_box && p_box->i_type != ATOM_root )
            {
               p_box = p_box->p_father;
            }
            if( !p_box )
            {
               goto error_box;
            }
         }
         else
            if( !strcmp( psz_token, "." ) )
            {
               /* Do nothing */
            }
            else
               if( !strcmp( psz_token, ".." ) )
               {
                  p_box = p_box->p_father;
                  if( !p_box )
                  {
                     goto error_box;
                  }
               }
               else
                  if( strlen( psz_token ) == 4 )
                  {
                     uint32_t i_fourcc;
                     i_fourcc = MP4_FOURCC( psz_token[0], psz_token[1],
                        psz_token[2], psz_token[3] );
                     p_box = p_box->p_first;
                     for( ; ; )
                     {
                        if( !p_box )
                        {
                           goto error_box;
                        }
                        if( p_box->i_type == i_fourcc )
                        {
                           if( !i_number )
                           {
                              break;
                           }
                           i_number--;
                        }
                        p_box = p_box->p_next;
                     }
                  }
                  else
                     if( *psz_token == '\0' )
                     {
                        p_box = p_box->p_first;
                        for( ; ; )
                        {
                           if( !p_box )
                           {
                              goto error_box;
                           }
                           if( !i_number )
                           {
                              break;
                           }
                           i_number--;
                           p_box = p_box->p_next;
                        }
                     }
                     else
                     {
                        //            fprintf( stderr, "Argg malformed token \"%s\"",psz_token );
                        goto error_box;
                     }

                     FREENULL( psz_token );
   }

   return;

error_box:
   free( psz_token );
   free( psz_dup );
   *pp_result = NULL;
   return;
}

mp4_box_t * MP4_BoxGet(mp4_box_t *p_box, const char *psz_fmt, ...)
{
   va_list args;
   mp4_box_t *p_result;

   va_start( args, psz_fmt );
   MP4_BoxGet_Internal( &p_result, p_box, psz_fmt, args );
   va_end( args );

   return( p_result );
}

