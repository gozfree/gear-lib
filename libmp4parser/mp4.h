/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    mp4.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-27 18:22
 * updated: 2016-07-27 18:22
 ******************************************************************************/
#ifndef __MP4_H__
#define __MP4_H__

#include "stream.h"


#define MP4_FOURCC( a, b, c, d ) \
   ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
   | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )

#define ATOM_root MP4_FOURCC( 'r', 'o', 'o', 't' )
#define ATOM_uuid MP4_FOURCC( 'u', 'u', 'i', 'd' )

#define ATOM_ftyp MP4_FOURCC( 'f', 't', 'y', 'p' )
#define ATOM_mmpu MP4_FOURCC( 'm', 'm', 'p', 'u' )
#define ATOM_tfdt MP4_FOURCC( 't', 'f', 'd', 't' )
#define ATOM_moov MP4_FOURCC( 'm', 'o', 'o', 'v' )
#define ATOM_foov MP4_FOURCC( 'f', 'o', 'o', 'v' )
#define ATOM_cmov MP4_FOURCC( 'c', 'm', 'o', 'v' )
#define ATOM_dcom MP4_FOURCC( 'd', 'c', 'o', 'm' )
#define ATOM_cmvd MP4_FOURCC( 'c', 'm', 'v', 'd' )

#define ATOM_moof MP4_FOURCC( 'm', 'o', 'o', 'f' )
#define ATOM_mdat MP4_FOURCC( 'm', 'd', 'a', 't' )
#define ATOM_skip MP4_FOURCC( 's', 'k', 'i', 'p' )
#define ATOM_free MP4_FOURCC( 'f', 'r', 'e', 'e' )
#define ATOM_udta MP4_FOURCC( 'u', 'd', 't', 'a' )
#define ATOM_wide MP4_FOURCC( 'w', 'i', 'd', 'e' )

#define ATOM_data MP4_FOURCC( 'd', 'a', 't', 'a' )

#define ATOM_trak MP4_FOURCC( 't', 'r', 'a', 'k' )
#define ATOM_mvhd MP4_FOURCC( 'm', 'v', 'h', 'd' )
#define ATOM_tkhd MP4_FOURCC( 't', 'k', 'h', 'd' )
#define ATOM_hint MP4_FOURCC( 'h', 'i', 'n', 't' )
#define ATOM_tref MP4_FOURCC( 't', 'r', 'e', 'f' )
#define ATOM_mdia MP4_FOURCC( 'm', 'd', 'i', 'a' )
#define ATOM_mdhd MP4_FOURCC( 'm', 'd', 'h', 'd' )
#define ATOM_hdlr MP4_FOURCC( 'h', 'd', 'l', 'r' )
#define ATOM_minf MP4_FOURCC( 'm', 'i', 'n', 'f' )
#define ATOM_vmhd MP4_FOURCC( 'v', 'm', 'h', 'd' )
#define ATOM_smhd MP4_FOURCC( 's', 'm', 'h', 'd' )
#define ATOM_hmhd MP4_FOURCC( 'h', 'm', 'h', 'd' )
#define ATOM_dinf MP4_FOURCC( 'd', 'i', 'n', 'f' )
#define ATOM_url  MP4_FOURCC( 'u', 'r', 'l', ' ' )
#define ATOM_urn  MP4_FOURCC( 'u', 'r', 'n', ' ' )
#define ATOM_dref MP4_FOURCC( 'd', 'r', 'e', 'f' )
#define ATOM_stbl MP4_FOURCC( 's', 't', 'b', 'l' )
#define ATOM_stts MP4_FOURCC( 's', 't', 't', 's' )
#define ATOM_ctts MP4_FOURCC( 'c', 't', 't', 's' )
#define ATOM_stsd MP4_FOURCC( 's', 't', 's', 'd' )
#define ATOM_stsz MP4_FOURCC( 's', 't', 's', 'z' )
#define ATOM_stz2 MP4_FOURCC( 's', 't', 'z', '2' )
#define ATOM_stsc MP4_FOURCC( 's', 't', 's', 'c' )
#define ATOM_stco MP4_FOURCC( 's', 't', 'c', 'o' )
#define ATOM_co64 MP4_FOURCC( 'c', 'o', '6', '4' )
#define ATOM_stss MP4_FOURCC( 's', 't', 's', 's' )
#define ATOM_stsh MP4_FOURCC( 's', 't', 's', 'h' )
#define ATOM_stdp MP4_FOURCC( 's', 't', 'd', 'p' )
#define ATOM_padb MP4_FOURCC( 'p', 'a', 'd', 'b' )
#define ATOM_edts MP4_FOURCC( 'e', 'd', 't', 's' )
#define ATOM_elst MP4_FOURCC( 'e', 'l', 's', 't' )
#define ATOM_mvex MP4_FOURCC( 'm', 'v', 'e', 'x' )
#define ATOM_sdtp MP4_FOURCC( 's', 'd', 't', 'p' )
#define ATOM_trex MP4_FOURCC( 't', 'r', 'e', 'x' )
#define ATOM_mehd MP4_FOURCC( 'm', 'e', 'h', 'd' )
#define ATOM_mfhd MP4_FOURCC( 'm', 'f', 'h', 'd' )
#define ATOM_traf MP4_FOURCC( 't', 'r', 'a', 'f' )
#define ATOM_tfhd MP4_FOURCC( 't', 'f', 'h', 'd' )
#define ATOM_trun MP4_FOURCC( 't', 'r', 'u', 'n' )
#define ATOM_cprt MP4_FOURCC( 'c', 'p', 'r', 't' )
#define ATOM_iods MP4_FOURCC( 'i', 'o', 'd', 's' )
#define ATOM_pasp MP4_FOURCC( 'p', 'a', 's', 'p' )
#define ATOM_mfra MP4_FOURCC( 'm', 'f', 'r', 'a' )
#define ATOM_mfro MP4_FOURCC( 'm', 'f', 'r', 'o' )
#define ATOM_tfra MP4_FOURCC( 't', 'f', 'r', 'a' )

#define ATOM_nmhd MP4_FOURCC( 'n', 'm', 'h', 'd' )
#define ATOM_mp2v MP4_FOURCC( 'm', 'p', '2', 'v' )
#define ATOM_mp4v MP4_FOURCC( 'm', 'p', '4', 'v' )
#define ATOM_mp4a MP4_FOURCC( 'm', 'p', '4', 'a' )
#define ATOM_mp4s MP4_FOURCC( 'm', 'p', '4', 's' )
#define ATOM_vide MP4_FOURCC( 'v', 'i', 'd', 'e' )
#define ATOM_soun MP4_FOURCC( 's', 'o', 'u', 'n' )
#define ATOM_mmth MP4_FOURCC( 'm', 'm', 't', 'h' )
#define ATOM_hint MP4_FOURCC( 'h', 'i', 'n', 't' )
#define ATOM_hdv2 MP4_FOURCC( 'h', 'd', 'v', '2' )

#define ATOM_dpnd MP4_FOURCC( 'd', 'p', 'n', 'd' )
#define ATOM_ipir MP4_FOURCC( 'i', 'p', 'i', 'r' )
#define ATOM_mpod MP4_FOURCC( 'm', 'p', 'o', 'd' )
#define ATOM_hnti MP4_FOURCC( 'h', 'n', 't', 'i' )
#define ATOM_rtp  MP4_FOURCC( 'r', 't', 'p', ' ' )

#define ATOM_isom MP4_FOURCC( 'i', 's', 'o', 'm' )
#define ATOM_3gp4 MP4_FOURCC( '3', 'g', 'p', '4' )
#define ATOM_esds MP4_FOURCC( 'e', 's', 'd', 's' )

#define ATOM__mp3 MP4_FOURCC( '.', 'm', 'p', '3' )
#define ATOM_ms02 MP4_FOURCC( 'm', 's', 0x0, 0x02 )
#define ATOM_ms11 MP4_FOURCC( 'm', 's', 0x0, 0x11 )
#define ATOM_ms55 MP4_FOURCC( 'm', 's', 0x0, 0x55 )
#define ATOM_twos MP4_FOURCC( 't', 'w', 'o', 's' )
#define ATOM_sowt MP4_FOURCC( 's', 'o', 'w', 't' )
#define ATOM_QDMC MP4_FOURCC( 'Q', 'D', 'M', 'C' )
#define ATOM_QDM2 MP4_FOURCC( 'Q', 'D', 'M', '2' )
#define ATOM_ima4 MP4_FOURCC( 'i', 'm', 'a', '4' )
#define ATOM_IMA4 MP4_FOURCC( 'I', 'M', 'A', '4' )
#define ATOM_dvi  MP4_FOURCC( 'd', 'v', 'i', ' ' )
#define ATOM_MAC3 MP4_FOURCC( 'M', 'A', 'C', '3' )
#define ATOM_MAC6 MP4_FOURCC( 'M', 'A', 'C', '6' )
#define ATOM_alaw MP4_FOURCC( 'a', 'l', 'a', 'w' )
#define ATOM_ulaw MP4_FOURCC( 'u', 'l', 'a', 'w' )
#define ATOM_Qclp MP4_FOURCC( 'Q', 'c', 'l', 'p' )
#define ATOM_samr MP4_FOURCC( 's', 'a', 'm', 'r' )
#define ATOM_sawb MP4_FOURCC( 's', 'a', 'w', 'b' )
#define ATOM_OggS MP4_FOURCC( 'O', 'g', 'g', 'S' )
#define ATOM_alac MP4_FOURCC( 'a', 'l', 'a', 'c' )
#define ATOM_dac3 MP4_FOURCC( 'd', 'a', 'c', '3' )
#define ATOM_dec3 MP4_FOURCC( 'd', 'e', 'c', '3' )
#define ATOM_enda MP4_FOURCC( 'e', 'n', 'd', 'a' )
#define ATOM_gnre MP4_FOURCC( 'g', 'n', 'r', 'e' )
#define ATOM_trkn MP4_FOURCC( 't', 'r', 'k', 'n' )

#define ATOM_zlib MP4_FOURCC( 'z', 'l', 'i', 'b' )
#define ATOM_SVQ1 MP4_FOURCC( 'S', 'V', 'Q', '1' )
#define ATOM_SVQ3 MP4_FOURCC( 'S', 'V', 'Q', '3' )
#define ATOM_ZyGo MP4_FOURCC( 'Z', 'y', 'G', 'o' )
#define ATOM_3IV1 MP4_FOURCC( '3', 'I', 'V', '1' )
#define ATOM_3iv1 MP4_FOURCC( '3', 'i', 'v', '1' )
#define ATOM_3IV2 MP4_FOURCC( '3', 'I', 'V', '2' )
#define ATOM_3iv2 MP4_FOURCC( '3', 'i', 'v', '2' )
#define ATOM_3IVD MP4_FOURCC( '3', 'I', 'V', 'D' )
#define ATOM_3ivd MP4_FOURCC( '3', 'i', 'v', 'd' )
#define ATOM_3VID MP4_FOURCC( '3', 'V', 'I', 'D' )
#define ATOM_3vid MP4_FOURCC( '3', 'v', 'i', 'd' )
#define ATOM_h263 MP4_FOURCC( 'h', '2', '6', '3' )
#define ATOM_s263 MP4_FOURCC( 's', '2', '6', '3' )
#define ATOM_DIVX MP4_FOURCC( 'D', 'I', 'V', 'X' )
#define ATOM_XVID MP4_FOURCC( 'X', 'V', 'I', 'D' )
#define ATOM_cvid MP4_FOURCC( 'c', 'v', 'i', 'd' )
#define ATOM_mjpa MP4_FOURCC( 'm', 'j', 'p', 'a' )
#define ATOM_mjpb MP4_FOURCC( 'm', 'j', 'q', 't' )
#define ATOM_mjqt MP4_FOURCC( 'm', 'j', 'h', 't' )
#define ATOM_mjht MP4_FOURCC( 'm', 'j', 'p', 'b' )
#define ATOM_VP31 MP4_FOURCC( 'V', 'P', '3', '1' )
#define ATOM_vp31 MP4_FOURCC( 'v', 'p', '3', '1' )
#define ATOM_h264 MP4_FOURCC( 'h', '2', '6', '4' )
#define ATOM_qdrw MP4_FOURCC( 'q', 'd', 'r', 'w' )

#define ATOM_avc1 MP4_FOURCC( 'a', 'v', 'c', '1' )
#define ATOM_avcC MP4_FOURCC( 'a', 'v', 'c', 'C' )
#define ATOM_hvcC MP4_FOURCC( 'h', 'v', 'c', 'C' )
#define ATOM_m4ds MP4_FOURCC( 'm', '4', 'd', 's' )

#define ATOM_dvc  MP4_FOURCC( 'd', 'v', 'c', ' ' )
#define ATOM_dvp  MP4_FOURCC( 'd', 'v', 'p', ' ' )
#define ATOM_dv5n MP4_FOURCC( 'd', 'v', '5', 'n' )
#define ATOM_dv5p MP4_FOURCC( 'd', 'v', '5', 'p' )
#define ATOM_raw  MP4_FOURCC( 'r', 'a', 'w', ' ' )

#define ATOM_jpeg MP4_FOURCC( 'j', 'p', 'e', 'g' )

#define ATOM_yv12 MP4_FOURCC( 'y', 'v', '1', '2' )
#define ATOM_yuv2 MP4_FOURCC( 'y', 'u', 'v', '2' )

#define ATOM_rmra MP4_FOURCC( 'r', 'm', 'r', 'a' )
#define ATOM_rmda MP4_FOURCC( 'r', 'm', 'd', 'a' )
#define ATOM_rdrf MP4_FOURCC( 'r', 'd', 'r', 'f' )
#define ATOM_rmdr MP4_FOURCC( 'r', 'm', 'd', 'r' )
#define ATOM_rmvc MP4_FOURCC( 'r', 'm', 'v', 'c' )
#define ATOM_rmcd MP4_FOURCC( 'r', 'm', 'c', 'd' )
#define ATOM_rmqu MP4_FOURCC( 'r', 'm', 'q', 'u' )
#define ATOM_alis MP4_FOURCC( 'a', 'l', 'i', 's' )

#define ATOM_gmhd MP4_FOURCC( 'g', 'm', 'h', 'd' )
#define ATOM_wave MP4_FOURCC( 'w', 'a', 'v', 'e' )

#define ATOM_drms MP4_FOURCC( 'd', 'r', 'm', 's' )
#define ATOM_sinf MP4_FOURCC( 's', 'i', 'n', 'f' )
#define ATOM_schi MP4_FOURCC( 's', 'c', 'h', 'i' )
#define ATOM_user MP4_FOURCC( 'u', 's', 'e', 'r' )
#define ATOM_key  MP4_FOURCC( 'k', 'e', 'y', ' ' )
#define ATOM_iviv MP4_FOURCC( 'i', 'v', 'i', 'v' )
#define ATOM_name MP4_FOURCC( 'n', 'a', 'm', 'e' )
#define ATOM_priv MP4_FOURCC( 'p', 'r', 'i', 'v' )
#define ATOM_drmi MP4_FOURCC( 'd', 'r', 'm', 'i' )
#define ATOM_frma MP4_FOURCC( 'f', 'r', 'm', 'a' )
#define ATOM_skcr MP4_FOURCC( 's', 'k', 'c', 'r' )

#define ATOM_text MP4_FOURCC( 't', 'e', 'x', 't' )
#define ATOM_tx3g MP4_FOURCC( 't', 'x', '3', 'g' )
#define ATOM_subp MP4_FOURCC( 's', 'u', 'b', 'p' )
#define ATOM_sbtl MP4_FOURCC( 's', 'b', 't', 'l' )

#define ATOM_0xa9nam MP4_FOURCC( 0xa9, 'n', 'a', 'm' )
#define ATOM_0xa9aut MP4_FOURCC( 0xa9, 'a', 'u', 't' )
#define ATOM_0xa9cpy MP4_FOURCC( 0xa9, 'c', 'p', 'y' )
#define ATOM_0xa9inf MP4_FOURCC( 0xa9, 'i', 'n', 'f' )
#define ATOM_0xa9ART MP4_FOURCC( 0xa9, 'A', 'R', 'T' )
#define ATOM_0xa9des MP4_FOURCC( 0xa9, 'd', 'e', 's' )
#define ATOM_0xa9dir MP4_FOURCC( 0xa9, 'd', 'i', 'r' )
#define ATOM_0xa9cmt MP4_FOURCC( 0xa9, 'c', 'm', 't' )
#define ATOM_0xa9req MP4_FOURCC( 0xa9, 'r', 'e', 'q' )
#define ATOM_0xa9day MP4_FOURCC( 0xa9, 'd', 'a', 'y' )
#define ATOM_0xa9fmt MP4_FOURCC( 0xa9, 'f', 'm', 't' )
#define ATOM_0xa9prd MP4_FOURCC( 0xa9, 'p', 'r', 'd' )
#define ATOM_0xa9prf MP4_FOURCC( 0xa9, 'p', 'r', 'f' )
#define ATOM_0xa9src MP4_FOURCC( 0xa9, 's', 'r', 'c' )
#define ATOM_0xa9alb MP4_FOURCC( 0xa9, 'a', 'l', 'b' )
#define ATOM_0xa9dis MP4_FOURCC( 0xa9, 'd', 'i', 's' )
#define ATOM_0xa9enc MP4_FOURCC( 0xa9, 'e', 'n', 'c' )
#define ATOM_0xa9trk MP4_FOURCC( 0xa9, 't', 'r', 'k' )
#define ATOM_0xa9url MP4_FOURCC( 0xa9, 'u', 'r', 'l' )
#define ATOM_0xa9dsa MP4_FOURCC( 0xa9, 'd', 's', 'a' )
#define ATOM_0xa9hst MP4_FOURCC( 0xa9, 'h', 's', 't' )
#define ATOM_0xa9ope MP4_FOURCC( 0xa9, 'o', 'p', 'e' )
#define ATOM_0xa9wrt MP4_FOURCC( 0xa9, 'w', 'r', 't' )
#define ATOM_0xa9com MP4_FOURCC( 0xa9, 'c', 'o', 'm' )
#define ATOM_0xa9gen MP4_FOURCC( 0xa9, 'g', 'e', 'n' )
#define ATOM_0xa9too MP4_FOURCC( 0xa9, 't', 'o', 'o' )
#define ATOM_0xa9wrn MP4_FOURCC( 0xa9, 'w', 'r', 'n' )
#define ATOM_0xa9swr MP4_FOURCC( 0xa9, 's', 'w', 'r' )
#define ATOM_0xa9mak MP4_FOURCC( 0xa9, 'm', 'a', 'k' )
#define ATOM_0xa9mod MP4_FOURCC( 0xa9, 'm', 'o', 'd' )
#define ATOM_0xa9PRD MP4_FOURCC( 0xa9, 'P', 'R', 'D' )
#define ATOM_0xa9grp MP4_FOURCC( 0xa9, 'g', 'r', 'p' )
#define ATOM_0xa9lyr MP4_FOURCC( 0xa9, 'g', 'r', 'p' )
#define ATOM_chpl MP4_FOURCC( 'c', 'h', 'p', 'l' )
#define ATOM_WLOC MP4_FOURCC( 'W', 'L', 'O', 'C' )

#define ATOM_meta MP4_FOURCC( 'm', 'e', 't', 'a' )
#define ATOM_ilst MP4_FOURCC( 'i', 'l', 's', 't' )

#define ATOM_chap MP4_FOURCC( 'c', 'h', 'a', 'p' )



typedef struct uuid_s
{
   uint8_t b[16];
} uuid_t;

/* specific structure for all boxes */
typedef struct mp4_box_data_ftyp_s
{
   uint32_t major_brand;
   uint32_t minor_version;

   uint32_t compatible_brands_count;
   uint32_t *compatible_brands;

} mp4_box_data_ftyp_t;

typedef struct mp4_box_data_mmpu_s
{
	uint8_t  version;
	uint32_t flags;
	uint8_t  is_complete;
	uint8_t  reserved;
   uint32_t mpu_sequence_number;
   uint32_t asset_id_scheme;
   uint32_t asset_id_length;
   char *asset_id_value;

} mp4_box_data_mmpu_t;

typedef struct mp4_box_data_tfdt_s
{
	uint8_t  version;
	uint32_t flags;
    uint64_t baseMediaDecodeTime;

} mp4_box_data_tfdt_t;

typedef struct mp4_box_data_mvhd_s
{
   uint8_t  version;
   uint32_t flags;

   uint64_t creation_time;
   uint64_t modification_time;
   uint32_t timescale;
   uint64_t duration;

   int32_t  rate;
   int16_t  volume;
   int16_t  reserved1;
   uint32_t reserved2[2];
   int32_t  matrix[9];
   uint32_t predefined[6];
   uint32_t next_track_id;

} mp4_box_data_mvhd_t;

#define MP4_TRACK_ENABLED    0x000001
#define MP4_TRACK_IN_MOVIE   0x000002
#define MP4_TRACK_IN_PREVIEW 0x000004

typedef struct mp4_box_data_tkhd_s
{
   uint8_t  version;
   uint32_t flags;

   uint64_t creation_time;
   uint64_t modification_time;
   uint32_t track_id;
   uint32_t reserved;
   uint64_t duration;

   uint32_t reserved2[2];
   int16_t  layer;
   int16_t  predefined;

   int16_t  volume;
   uint16_t reserved3;
   int32_t  matrix[9];
   int32_t  width;
   int32_t  height;

} mp4_box_data_tkhd_t;

typedef struct mp4_box_data_hint_s
{

	uint32_t track_IDs;

} mp4_box_data_hint_t;

typedef struct mp4_box_data_mdhd_s
{
   uint8_t  version;
   uint32_t flags;

   uint64_t creation_time;
   uint64_t modification_time;
   uint32_t timescale;
   uint64_t duration;

   /* one bit for pad */
   uint16_t language_code;
   /* unsigned int(5)[3] language difference with 0x60*/
   unsigned char language[3];
   uint16_t predefined;

} mp4_box_data_mdhd_t;

typedef struct mp4_box_data_hdlr_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t predefined;
    uint32_t handler_type; /* "vide" "soun" "hint" "odsm" "crsm" "sdsm" "m7sm" "ocsm" "ipsm" "mjsm" */

    unsigned char *psz_name; /* in UTF-8 */

} mp4_box_data_hdlr_t;

typedef struct mp4_box_data_vmhd_s
{
   uint8_t  version;
   uint32_t flags;

   int16_t  graphics_mode;
   int16_t  opcolor[3];

} mp4_box_data_vmhd_t;

typedef struct mp4_box_data_smhd_s
{
   uint8_t  version;
   uint32_t flags;

   int16_t  balance;
   int16_t  reserved;

} mp4_box_data_smhd_t;

typedef struct mp4_box_data_hmhd_s
{
   uint8_t  version;
   uint32_t flags;

   uint16_t max_PDU_size;
   uint16_t avg_PDU_size;
   uint32_t max_bitrate;
   uint32_t avg_bitrate;
   uint32_t reserved;

} mp4_box_data_hmhd_t;

typedef struct mp4_box_data_url_s
{
   uint8_t  version;
   uint32_t flags;

   char *psz_location;

} mp4_box_data_url_t;

typedef struct mp4_box_data_urn_s
{
    uint8_t  version;
    uint32_t flags;

    char *psz_name;
    char *psz_location;

} mp4_box_data_urn_t;

typedef struct mp4_box_data_dref_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;
/* XXX it's also a container with entry_count entry */
} mp4_box_data_dref_t;

typedef struct mp4_box_data_stts_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;
    uint32_t *sample_count; /* these are array */
    int32_t  *sample_delta;

} mp4_box_data_stts_t;

typedef struct mp4_box_data_ctts_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    uint32_t *sample_count; /* these are array */
    int32_t  *sample_offset;

} mp4_box_data_ctts_t;


typedef struct mp4_box_data_sample_soun_s
{
    uint8_t  reserved1[6];
    uint16_t data_reference_index;

    //uint32_t reserved2[2];
    uint16_t qt_version;
    uint16_t qt_revision_level;
    uint32_t qt_vendor;

    uint16_t channelcount;
    uint16_t samplesize;
    uint16_t predefined;
    uint16_t reserved3;
    uint16_t sampleratehi; /* timescale of track */
    uint16_t sampleratelo;

    /* for version 1 (reserved1[0] == 1) */
    uint32_t sample_per_packet;
    uint32_t bytes_per_packet;
    uint32_t bytes_per_frame;
    uint32_t bytes_per_sample;

    /* XXX hack */
    int     qt_description;
    uint8_t *p_qt_description;

    void    *drms;

} mp4_box_data_sample_soun_t;

typedef struct mp4_box_data_sample_vide_s
{
    uint8_t  reserved1[6];
    uint16_t data_reference_index;

    uint16_t qt_version;
    uint16_t qt_revision_level;
    uint32_t qt_vendor;

    uint32_t qt_temporal_quality;
    uint32_t qt_spatial_quality;

    int16_t  width;
    int16_t  height;

    uint32_t horizresolution;
    uint32_t vertresolution;

    uint32_t qt_data_size;
    uint16_t qt_frame_count;

    uint8_t  compressorname[32];
    int16_t  depth;

    int16_t  qt_color_table;

    /* XXX hack ImageDescription */
    int     qt_image_description;
    uint8_t *p_qt_image_description;

    void    *drms;

} mp4_box_data_sample_vide_t;

typedef struct mp4_box_data_sample_mmth_s
{
    uint8_t  reserved1[6];
    uint16_t data_reference_index;

    uint16_t hinttrackversion;
    uint16_t highestcompatibleversion;
    uint16_t packet_id;
    uint8_t has_mfus_flag;
    uint8_t is_timed;
    uint8_t reserved;
    /*uint32_t asset_id_scheme;
    uint32_t asset_id_length;
    char *asset_id_value;*/

} mp4_box_data_sample_mmth_t;

typedef struct mp4_box_data_mmthsample_s
{
	uint32_t sequence_number;
	uint8_t trackrefindex;
	uint32_t moviefragmentsequencenumber;
	uint32_t samplenumber;
	uint8_t priority;
	uint8_t dependency_counter;
	uint32_t offset;
	uint32_t length;
	uint16_t item_ID;

} mp4_box_data_mmthsample_t;

typedef struct mp4_box_data_muli_s
{
	uint8_t multilayer_flag:1;
	uint8_t reserved0:7;
	uint8_t dependency_id:3;
	uint8_t depth_flag:1;
	uint8_t reserved1:4;
	uint8_t temporal_id:3;
	uint8_t reserved2:1;
	uint8_t quality_id:4;
	uint8_t priority_id:6;
	uint16_t view_id:10;

	uint8_t layer_id:6;
	//uint8_t temporal_id:3;
	uint8_t reserved3:7;

} mp4_box_data_muli_t;



#define MP4_TEXT_DISPLAY_FLAG_DONT_DISPLAY       (1<<0)
#define MP4_TEXT_DISPLAY_FLAG_AUTO_SCALE         (1<<1)
#define MP4_TEXT_DISPLAY_FLAG_CLIP_TO_TEXT_BOX   (1<<2)
#define MP4_TEXT_DISPLAY_FLAG_USE_MOVIE_BG_COLOR (1<<3)
#define MP4_TEXT_DISPLAY_FLAG_SHRINK_TEXT_BOX_TO_FIT (1<<4)
#define MP4_TEXT_DISPLAY_FLAG_SCROLL_IN          (1<<5)
#define MP4_TEXT_DISPLAY_FLAG_SCROLL_OUT         (1<<6)
#define MP4_TEXT_DISPLAY_FLAG_HORIZONTAL_SCROLL  (1<<7)
#define MP4_TEXT_DISPLAY_FLAG_REVERSE_SCROLL     (1<<8)
#define MP4_TEXT_DISPLAY_FLAG_CONTINUOUS_SCROLL  (1<<9)
#define MP4_TEXT_DISPLAY_FLAG_FLOW_HORIZONTAL    (1<<10)
#define MP4_TEXT_DISPLAY_FLAG_CONTINUOUS_KARAOKE (1<<11)
#define MP4_TEXT_DISPLAY_FLAG_DROP_SHADOW        (1<<12)
#define MP4_TEXT_DISPLAY_FLAG_ANTI_ALIAS         (1<<13)
#define MP4_TEXT_DISPLAY_FLAG_KEYED_TEXT         (1<<14)
#define MP4_TEXT_DISPLAY_FLAG_INVERSE_HILITE     (1<<15)
#define MP4_TEXT_DISPLAY_FLAG_COLOR_HILITE       (1<<16)
#define MP4_TEXT_DISPLAY_FLAG_WRITE_VERTICALLY   (1<<17)

typedef struct
{
    uint32_t reserved1;
    uint16_t reserved2;

    uint16_t data_reference_index;

    uint32_t display_flags;   // TextDescription and Tx3gDescription

    int8_t justification_horizontal; // left(0), centered(1), right(-1)
    int8_t justification_vertical;   // top(0), centered(1), bottom(-1)

    uint16_t background_color[4];

    uint16_t text_box_top;
    uint16_t text_box_left;
    uint16_t text_box_bottom;
    uint16_t text_box_right;

    // TODO to complete
} mp4_box_data_sample_text_t;

typedef struct mp4_box_data_sample_hint_s
{
    uint8_t  reserved1[6];
    uint16_t data_reference_index;

    uint8_t *data;

} mp4_box_data_sample_hint_t;

typedef struct mp4_box_data_moviehintinformation_rtp_s
{
    uint32_t description_format;
    unsigned char *psz_text;

} mp4_box_data_moviehintinformation_rtp_t;



typedef struct mp4_box_data_stsd_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    /* it contains SampleEntry handled as if it was Box */

} mp4_box_data_stsd_t;


typedef struct mp4_box_data_stsz_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t sample_size;
    uint32_t sample_count;

    uint32_t *entry_size; /* array , empty if sample_size != 0 */

} mp4_box_data_stsz_t;

typedef struct mp4_box_data_stz2_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t sample_size; /* 24 bits */
    uint8_t  field_size;
    uint32_t sample_count;

    uint32_t *entry_size; /* array: unsigned int(field_size) entry_size */

} mp4_box_data_stz2_t;

typedef struct mp4_box_data_stsc_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    uint32_t *first_chunk; /* theses are arrays */
    uint32_t *samples_per_chunk;
    uint32_t *sample_description_index;

} mp4_box_data_stsc_t;


typedef struct mp4_box_data_co64_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    uint64_t *chunk_offset;

} mp4_box_data_co64_t;


typedef struct mp4_box_data_stss_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    uint32_t *sample_number;

} mp4_box_data_stss_t;

typedef struct mp4_box_data_stsh_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    uint32_t *shadowed_sample_number;
    uint32_t *sync_sample_number;

} mp4_box_data_stsh_t;

typedef struct mp4_box_data_stdp_s
{
    uint8_t  version;
    uint32_t flags;

    uint16_t *priority;

} mp4_box_data_stdp_t;

typedef struct mp4_box_data_padb_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t sample_count;

    uint16_t *reserved1;   /* 1bit  */
    uint16_t *pad2;        /* 3bits */
    uint16_t *reserved2;   /* 1bit  */
    uint16_t *pad1;        /* 3bits */


} mp4_box_data_padb_t;


typedef struct mp4_box_data_elst_s
{
    uint8_t  version;
    uint32_t flags;

    uint32_t entry_count;

    uint64_t *segment_duration;
    int64_t  *media_time;
    uint16_t *media_rate_integer;
    uint16_t *media_rate_fraction;


} mp4_box_data_elst_t;

typedef struct mp4_box_data_cprt_s
{
    uint8_t  version;
    uint32_t flags;
    /* 1 pad bit */
    unsigned char language[3];

    char *psz_notice;
} mp4_box_data_cprt_t;


/* DecoderConfigDescriptor */
typedef struct mp4_descriptor_decoder_config_s
{
    uint8_t objectTypeIndication;
    uint8_t streamType;
    int     b_upStream;
    int     buffer_sizeDB;
    int     max_bitrate;
    int     avg_bitrate;

    int     decoder_specific_info_len;
    uint8_t *decoder_specific_info;
    /* some other stuff */

} mp4_descriptor_decoder_config_t;

typedef struct mp4_descriptor_SL_config_s
{

    int dummy; /* ANSI C forbids empty structures */

} mp4_descriptor_SL_config_t;


typedef struct mp4_descriptor_ES_s
{
    uint16_t ES_ID;
    int      b_stream_dependence;
    int      b_url;
    int      b_OCRstream;
    int      stream_priority;

    int      depend_on_ES_ID; /* if b_stream_dependence set */

    unsigned char *psz_URL;

    uint16_t OCR_ES_ID;       /* if b_OCRstream */
    mp4_descriptor_decoder_config_t *decConfigDescr;

    mp4_descriptor_SL_config_t *slConfigDescr;

    /* some other stuff ... */

} mp4_descriptor_ES_t;

/* ES descriptor */
typedef struct mp4_box_data_esds_s
{
    uint8_t  version;
    uint32_t flags;

    mp4_descriptor_ES_t es_descriptor;

} mp4_box_data_esds_t;


typedef struct mp4_box_data_dcom_s
{
    uint32_t algorithm; /* fourcc */

} mp4_box_data_dcom_t;

typedef struct mp4_box_data_cmvd_s
{
    uint32_t uncompressed_size;
    uint32_t compressed_size;

    int     b_compressed; /* Set to 1 if compressed data, 0 if uncompressed */
    uint8_t *data;

} mp4_box_data_cmvd_t;

typedef struct mp4_box_data_cmov_s
{
    struct mp4_box_s *moov; /* uncompressed moov */

} mp4_box_data_cmov_t;

typedef struct
{
    uint32_t type;
} mp4_box_data_frma_t;

typedef struct
{
    uint32_t init;
    uint32_t encr;
    uint32_t decr;
} mp4_box_data_skcr_t;

typedef struct
{
    uint8_t  version;
    uint32_t flags;

    uint32_t ref_type;
    char     *psz_ref;

} mp4_box_data_rdrf_t;

typedef struct
{
    uint8_t  version;
    uint32_t flags;

    uint32_t rate;

} mp4_box_data_rmdr_t;

typedef struct
{
    uint8_t  version;
    uint32_t flags;

    uint32_t gestaltType;
    uint32_t val1;
    uint32_t val2;
    uint16_t checkType;   /* 0: val1 is version min
                               1: gestalt value & val2 == val1 */

} mp4_box_data_rmvc_t;

typedef struct
{
    uint8_t  version;
    uint32_t flags;


} mp4_box_data_rmcd_t;

typedef struct
{
    uint32_t quality;

} mp4_box_data_rmqu_t;

typedef struct mp4_box_data_mfhd_s
{
    uint32_t sequence_number;

    uint8_t *vendor_extension;

} mp4_box_data_mfhd_t;

#define MP4_TFHD_BASE_DATA_OFFSET     (1LL<<0)
#define MP4_TFHD_SAMPLE_DESC_INDEX    (1LL<<1)
#define MP4_TFHD_DFLT_SAMPLE_DURATION (1LL<<3)
#define MP4_TFHD_DFLT_SAMPLE_SIZE     (1LL<<4)
#define MP4_TFHD_DFLT_SAMPLE_FLAGS    (1LL<<5)
typedef struct mp4_box_data_tfhd_s
{
    uint8_t  version;
    uint32_t flags;
    uint32_t track_ID;

    /* optional fields */
    uint64_t base_data_offset;
    uint32_t sample_description_index;
    uint32_t default_sample_duration;
    uint32_t default_sample_size;
    uint32_t default_sample_flags;

} mp4_box_data_tfhd_t;

#define MP4_TRUN_DATA_OFFSET         (1<<0)
#define MP4_TRUN_FIRST_FLAGS         (1<<2)
#define MP4_TRUN_SAMPLE_DURATION     (1<<8)
#define MP4_TRUN_SAMPLE_SIZE         (1<<9)
#define MP4_TRUN_SAMPLE_FLAGS        (1<<10)
#define MP4_TRUN_SAMPLE_TIME_OFFSET  (1<<11)
typedef struct mp4_descriptor_trun_sample_t
{
    uint32_t duration;
    uint32_t size;
    uint32_t flags;
    uint32_t composition_time_offset;
} mp4_descriptor_trun_sample_t;

typedef struct mp4_box_data_trun_s
{
    uint8_t  version;
    uint32_t flags;
    uint32_t sample_count;

    /* optional fields */
    uint32_t data_offset;
    uint32_t first_sample_flags;

    mp4_descriptor_trun_sample_t *samples;

} mp4_box_data_trun_t;



typedef struct mp4_box_data_0xa9xxx_s
{
   char *psz_text;

} mp4_box_data_0xa9xxx_t;

typedef struct mp4_box_data_name_s
{
   char *psz_text;

} mp4_box_data_name_t;

typedef struct mp4_box_data_tref_generic_s
{
   uint32_t entry_count;
   uint32_t *track_ID;

} mp4_box_data_tref_generic_t;

typedef struct mp4_box_data_chpl_s
{
   uint8_t  version;
   uint32_t flags;

   uint8_t chapter;
   struct
   {
      char    *psz_name;
      int64_t  start;
   } p_chapter[256];
} mp4_box_data_chpl_t;

typedef struct mp4_box_data_avcC_s
{
   uint8_t version;
   uint8_t profile;
   uint8_t profile_compatibility;
   uint8_t level;

   uint8_t reserved1;     /* 6 bits */
   uint8_t length_size;

   uint8_t reserved2;    /* 3 bits */
   uint8_t  sps;
   uint16_t *sps_length;
   uint8_t  **p_sps;

   uint8_t  pps;
   uint16_t *pps_length;
   uint8_t  **p_pps;

   /* XXX: Hack raw avcC atom payload */
   int     avcC;
   uint8_t *p_avcC;

} mp4_box_data_avcC_t;

typedef struct mp4_box_data_dac3_s
{
   uint8_t fscod;
   uint8_t bsid;
   uint8_t bsmod;
   uint8_t acmod;
   uint8_t lfeon;
   uint8_t bitrate_code;

} mp4_box_data_dac3_t;

typedef struct mp4_box_data_enda_s
{
   uint16_t little_endian;

} mp4_box_data_enda_t;

typedef struct mp4_box_data_gnre_s
{
   uint16_t genre;

} mp4_box_data_gnre_t;

typedef struct mp4_box_data_trkn_s
{
   uint32_t track_number;
   uint32_t track_total;

} mp4_box_data_trkn_t;

typedef struct mp4_box_data_iods_s
{
   uint8_t  version;
   uint32_t flags;

   uint16_t object_descriptor;
   uint8_t OD_profile_level;
   uint8_t scene_profile_level;
   uint8_t audio_profile_level;
   uint8_t visual_profile_level;
   uint8_t graphics_profile_level;

} mp4_box_data_iods_t;

typedef struct mp4_box_data_pasp_s
{
   uint32_t horizontal_spacing;
   uint32_t vertical_spacing;
} mp4_box_data_pasp_t;

typedef struct mp4_box_data_mehd_s
{
   uint8_t  version;
   uint32_t flags;

   uint64_t fragment_duration;
} mp4_box_data_mehd_t;

typedef struct mp4_box_data_trex_s
{
   uint8_t  version;
   uint32_t flags;

   uint32_t track_ID;
   uint32_t default_sample_description_index;
   uint32_t default_sample_duration;
   uint32_t default_sample_size;
   uint32_t default_sample_flags;
} mp4_box_data_trex_t;

typedef struct mp4_box_data_sdtp_s
{
   uint8_t  version;
   uint32_t flags;

   uint8_t *sample_table;
} mp4_box_data_sdtp_t;

typedef struct mp4_box_data_mfro_s
{
   uint8_t  version;
   uint32_t flags;

   uint32_t size;
} mp4_box_data_mfro_t;

typedef struct mp4_box_data_tfra_s
{
   uint8_t  version;
   uint32_t flags;

   uint32_t track_ID;
   uint32_t number_of_entries;

   uint8_t length_size_of_traf_num;
   uint8_t length_size_of_trun_num;
   uint8_t length_size_of_sample_num;

   uint32_t *time;
   uint32_t *moof_offset;
   uint8_t *traf_number;
   uint8_t *trun_number;
   uint8_t *sample_number;
} mp4_box_data_tfra_t;

typedef union mp4_box_data_s
{
   mp4_box_data_ftyp_t *p_ftyp;
   mp4_box_data_mmpu_t *p_mmpu;
   mp4_box_data_tfdt_t *p_tfdt;
   mp4_box_data_mvhd_t *p_mvhd;
   mp4_box_data_mfhd_t *p_mfhd;
   mp4_box_data_tfhd_t *p_tfhd;
   mp4_box_data_trun_t *p_trun;
   mp4_box_data_tkhd_t *p_tkhd;
   mp4_box_data_hint_t *p_hint;
   mp4_box_data_mdhd_t *p_mdhd;
   mp4_box_data_hdlr_t *p_hdlr;
   mp4_box_data_vmhd_t *p_vmhd;
   mp4_box_data_smhd_t *p_smhd;
   mp4_box_data_hmhd_t *p_hmhd;
   mp4_box_data_url_t  *p_url;
   mp4_box_data_urn_t  *p_urn;
   mp4_box_data_dref_t *p_dref;
   mp4_box_data_stts_t *p_stts;
   mp4_box_data_ctts_t *p_ctts;
   mp4_box_data_stsd_t *p_stsd;
   mp4_box_data_sample_vide_t *p_sample_vide;
   mp4_box_data_sample_soun_t *p_sample_soun;
   mp4_box_data_sample_text_t *p_sample_text;
   mp4_box_data_sample_hint_t *p_sample_hint;
   mp4_box_data_sample_mmth_t *p_sample_mmth;

   mp4_box_data_esds_t *p_esds;
   mp4_box_data_avcC_t *p_avcC;
   mp4_box_data_dac3_t *p_dac3;
   mp4_box_data_enda_t *p_enda;
   mp4_box_data_gnre_t *p_gnre;
   mp4_box_data_trkn_t *p_trkn;
   mp4_box_data_iods_t *p_iods;
   mp4_box_data_pasp_t *p_pasp;
   mp4_box_data_trex_t *p_trex;
   mp4_box_data_mehd_t *p_mehd;
   mp4_box_data_sdtp_t *p_sdtp;

   mp4_box_data_tfra_t *p_tfra;
   mp4_box_data_mfro_t *p_mfro;

   mp4_box_data_stsz_t *p_stsz;
   mp4_box_data_stz2_t *p_stz2;
   mp4_box_data_stsc_t *p_stsc;
   mp4_box_data_co64_t *p_co64;
   mp4_box_data_stss_t *p_stss;
   mp4_box_data_stsh_t *p_stsh;
   mp4_box_data_stdp_t *p_stdp;
   mp4_box_data_padb_t *p_padb;
   mp4_box_data_elst_t *p_elst;
   mp4_box_data_cprt_t *p_cprt;

   mp4_box_data_dcom_t *p_dcom;
   mp4_box_data_cmvd_t *p_cmvd;
   mp4_box_data_cmov_t *p_cmov;

   mp4_box_data_moviehintinformation_rtp_t p_moviehintinformation_rtp;

   mp4_box_data_frma_t *p_frma;
   mp4_box_data_skcr_t *p_skcr;

   mp4_box_data_rdrf_t *p_rdrf;
   mp4_box_data_rmdr_t *p_rmdr;
   mp4_box_data_rmqu_t *p_rmqu;
   mp4_box_data_rmvc_t *p_rmvc;

   mp4_box_data_0xa9xxx_t *p_0xa9xxx;
   mp4_box_data_chpl_t *p_chpl;
   mp4_box_data_tref_generic_t *p_tref_generic;
   mp4_box_data_name_t *p_name;
   void *p_data;
} mp4_box_data_t;

typedef struct mp4_box_s
{
   int64_t i_pos;
   uint32_t i_type;
   uint32_t i_shortsize;
   uuid_t i_uuid;
   uint64_t i_size;
   mp4_box_data_t data;
   struct mp4_box_s* p_father;
   struct mp4_box_s* p_first;
   struct mp4_box_s* p_last;
   struct mp4_box_s* p_next;
} mp4_box_t;

mp4_box_t *MP4_BoxGetRoot(stream_t * s);
mp4_box_t *MP4_BoxGetRootFromBuffer(stream_t * s, unsigned long filesize);
void MP4_BoxFree( mp4_box_t *box );
void MP4_BoxFreeFromBuffer( mp4_box_t *box );
mp4_box_t *MP4_BoxGet(mp4_box_t *p_box, const char *psz_fmt, ...);
mp4_box_t * MP4_BoxSearchBox(mp4_box_t *p_head, uint32_t i_type);

#endif // __MP4_H__
