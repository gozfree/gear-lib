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
#include "rtmp_h264.h"
#include "rtmp_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
67 42 e0 0a 89 95 42 c1 2c 80     67 sps
68 ce 05 8b 72                    68 pps

0100 0010 1110 0000 0000 1010 1000 1001 1001 0101 0100 0010 1100 0001 0010 1100 1000 0000 sps
FIELD                                  No. of BITS  VALUE      CodeNum   Desc
profile_idc                            8            01000010   66        u(8)
constraint_set0_flag                   1            1                    u(1)
constraint_set1_flag                   1            1                    u(1)
constraint_set2_flag                   1            1                    u(1)
constraint_set3_flag                   1            0                    u(1)
reserved_zero_4bits                    4            0000                 u(4)
level_idc                              8            00001010   10        u(8)
seq_parameter_set_id                   1            1          0         ue(v)
log2_max_frame_num_minus4              7            0001001    8         ue(v)
pic_order_cnt_type                     1            1          0         ue(v)
log2_max_pic_order_cnt_lsb_minus4      5            00101      4         ue(v)
num_ref_frames                         3            010                  ue(v)
gaps_in_frame_num_value_allowed_flag   1            1                    u(1)
pic_width_in_mbs_minus1                9            000010110  20        ue(v)
pic_height_in_map_units_minus1         9            000010010  16        ue(v)
frame_mbs_only_flag                    1            1          0         u(1)
mb_adaptive_frame_field_flag           1            1          0         u(1)
direct_8x8_inference_flag              1            0                    u(1)
frame_cropping_flag                    1            0                    u(1)
vui_parameters_present_flag            1            1          0         u(1)


1100 1110 0000 0101 1000 1011 0111 0010  pps
FIELD                                  No. of BITS  VALUE      CodeNum   Desc
pic_parameter_set_id                   1            1          0         ue(v)
seq_parameter_set_id                   1            1          0         ue(v)
entropy_coding_mode_flag               1            0                    ue(1)
pic_order_present_flag                 1            0                    ue(1)
num_slice_groups_minus1                1            1          0         ue(v)
num_ref_idx_l0_active_minus1           1            1          0         ue(v)
num_ref_idx_l1_active_minus1           1            1          0         ue(v)
weighted_pred_flag                     1            0                    ue(1)
weighted_bipred_idc                    2            00                   ue(2)
pic_init_qp_minus26                    7            0001011    10(-5)    se(v)
pic_init_qs_minus26                    7            0001011    10(-5)    se(v)
chroma_qp_index_offset                 3            011        2(-1)     se(v)
deblocking_filter_control_present_flag 1            1                    ue(1)
constrained_intra_pred_flag            1            0                    ue(1)
redundant_pic_cnt_present_flag         1            0                    ue(1)
*/

enum {
    H264_NAL_SLICE           = 1,
    H264_NAL_DPA             = 2,
    H264_NAL_DPB             = 3,
    H264_NAL_DPC             = 4,
    H264_NAL_IDR_SLICE       = 5,
    H264_NAL_SEI             = 6,
    H264_NAL_SPS             = 7,
    H264_NAL_PPS             = 8,
    H264_NAL_AUD             = 9,
    H264_NAL_END_SEQUENCE    = 10,
    H264_NAL_END_STREAM      = 11,
    H264_NAL_FILLER_DATA     = 12,
    H264_NAL_SPS_EXT         = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
};

static int read_bit(uint8_t *buf, int* value, uint8_t *bit_pos, int num)
{
    *value = 0;
    int i=0;
    for (int j = 0; j < num; j++) {
        if (*bit_pos == 8){
            *bit_pos = 0;
            i++;
        }
        if (*bit_pos == 0){
            if (buf[i] == 0x3 &&
               *(buf+i-1) == 0 &&
               *(buf+i-2) == 0)
                i++;
        }
        *value <<= 1;
        *value += buf[i] >> (7 -(*bit_pos)++) & 0x1;
    }
    return i;
}

static int parse_exp_codes(uint8_t *buf, int* value, uint8_t *bit_pos, uint8_t type)
{
    int leadingZeroBits = -1;
    int i=0;
    uint8_t j=*bit_pos;
    for (uint8_t b = 0; !b; leadingZeroBits++,j++) {
        if(j == 8){
            i++;
            j=0;
        }
        if (j == 0){
            if (buf[i] == 0x3 &&
                *(buf+i-1) == 0 &&
                *(buf+i-2) == 0)
                i++;
        }
        b = buf[i] >> (7 -j) & 0x1;
    }
    int codeNum = 0;
    i += read_bit(buf+i,  &codeNum, &j, leadingZeroBits);
    codeNum += (1 << leadingZeroBits) -1;
    if (type == 0) {
         //ue(v)
        *value = codeNum;
    }else if (type == 1) {
        //se(v)
        *value = (codeNum/2+1);
        if (codeNum %2 == 0){
            *value *= -1;
        }
    }
    *bit_pos = j;
    return i;
}

#define parse_ue(x,y,z)	parse_exp_codes((x),(y),(z),0)
#define parse_se(x,y,z)	parse_exp_codes((x),(y),(z),1)

static int parse_scaling_list(uint8_t *buf, int sizeofScalingList, uint8_t *bit_pos)
{
    int* scalingList = (int *)calloc(1, sizeofScalingList*sizeof(int));
    int lastScale = 8;
    int nextScale = 8;
    int delta_scale;
    int i = 0;
    for (int j =0; j<sizeofScalingList; j++ ){
        if (nextScale != 0){
            i += parse_se(buf, &delta_scale,bit_pos);
            nextScale = (lastScale+ delta_scale + 256)%256;
        }
        scalingList[j] = ( nextScale == 0 ) ? lastScale : nextScale;
        lastScale = scalingList[j];
    }
    free(scalingList);
    return i;
}

static int get_h264_info(uint8_t *buf, int len, struct rtmp_h264_info *pH264Info)
{
    memset(pH264Info,0,sizeof(struct rtmp_h264_info));
    uint8_t *pSPS = buf;
    unsigned char bit_pos = 0;
    int profile_idc; // u(8)
    int constraint_set; //u(8)
    int level_idc; //u(8)
    int seq_paramter_set_id = 0;
    int chroma_format_idc = 0;
    int residual_colour_tramsform_flag;
    int bit_depth_luma_minus8 = 0;
    int bit_depth_chroma_minus8 = 0;
    int qpprime_y_zero_transform_bypass_flag;
    int seq_scaling_matrix_present_flag;
    int seq_scaling_list_present_flag[8] ;
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int num_ref_frames;
    int gaps_in_frame_num_value_allowed_flag;
    int pic_width_in_mbs_minus1;
    int pic_height_in_map_units_minus1;
    int frame_mbs_only_flag;
    int direct_8x8_inference_flag;

    pSPS += read_bit(pSPS, &profile_idc,&bit_pos,8);
    pSPS += read_bit(pSPS, &constraint_set,&bit_pos,8);
    pSPS += read_bit(pSPS, &level_idc, &bit_pos,8);
    pSPS += parse_ue(pSPS,&seq_paramter_set_id, &bit_pos);
    if (profile_idc == 100 ||profile_idc == 110 ||profile_idc == 122 ||profile_idc == 144 ){
        pSPS += parse_ue(pSPS,&chroma_format_idc,&bit_pos);
        if (chroma_format_idc == 3)
            pSPS += read_bit(pSPS, &residual_colour_tramsform_flag, &bit_pos, 1);
        pSPS += parse_ue(pSPS,&bit_depth_luma_minus8,&bit_pos);
        pSPS += parse_ue(pSPS,&bit_depth_chroma_minus8,&bit_pos);
        pSPS += read_bit(pSPS,&qpprime_y_zero_transform_bypass_flag,&bit_pos, 1);
        pSPS += read_bit(pSPS,&seq_scaling_matrix_present_flag,&bit_pos, 1);
        if (seq_scaling_matrix_present_flag) {
            for (unsigned int i = 0; i<8; i++) {
                pSPS += read_bit(pSPS,&seq_scaling_list_present_flag[i],&bit_pos, 1);
                if (seq_scaling_list_present_flag[i]){
                    if (i < 6){
                        pSPS += parse_scaling_list(pSPS,16,&bit_pos);
                    }else{
                        pSPS += parse_scaling_list(pSPS,64,&bit_pos);
                    }
                }
            }
        }
    }
    pSPS += parse_ue(pSPS,&log2_max_frame_num_minus4,&bit_pos);
    pSPS += parse_ue(pSPS,&pic_order_cnt_type,&bit_pos);
    if (pic_order_cnt_type == 0) {
        pSPS += parse_ue(pSPS,&log2_max_pic_order_cnt_lsb_minus4,&bit_pos);
    }else if (pic_order_cnt_type == 1){
        int delta_pic_order_always_zero_flag;
        pSPS += read_bit(pSPS, &delta_pic_order_always_zero_flag, &bit_pos, 1);
        int offset_for_non_ref_pic;
        int offset_for_top_to_bottom_field;
        int num_ref_frames_in_pic_order_cnt_cycle;
        pSPS += parse_se(pSPS,&offset_for_non_ref_pic, &bit_pos);
        pSPS += parse_se(pSPS,&offset_for_top_to_bottom_field, &bit_pos);
        pSPS += parse_ue(pSPS,&num_ref_frames_in_pic_order_cnt_cycle, &bit_pos);
        int* offset_for_ref_frame = (int *)calloc(1, sizeof(int)*num_ref_frames_in_pic_order_cnt_cycle);
        for (int i =0;i < num_ref_frames_in_pic_order_cnt_cycle; i++ ){
            pSPS += parse_se(pSPS,offset_for_ref_frame+i, &bit_pos);
        }
        free(offset_for_ref_frame);
    }
    pSPS += parse_ue(pSPS,&num_ref_frames, &bit_pos);
    pSPS += read_bit(pSPS, &gaps_in_frame_num_value_allowed_flag, &bit_pos, 1);
    pSPS += parse_ue(pSPS,&pic_width_in_mbs_minus1,&bit_pos);
    pSPS += parse_ue(pSPS,&pic_height_in_map_units_minus1, &bit_pos);

    pH264Info->width = (short)(pic_width_in_mbs_minus1 + 1) << 4;
    pH264Info->height = (short)(pic_height_in_map_units_minus1 + 1) <<4;

    pSPS += read_bit(pSPS, &frame_mbs_only_flag, &bit_pos, 1);
    if (!frame_mbs_only_flag){
        int mb_adaptive_frame_field_flag;
        pSPS += read_bit(pSPS, &mb_adaptive_frame_field_flag, &bit_pos, 1);
        pH264Info->height *= 2;
    }

    pSPS += read_bit(pSPS, &direct_8x8_inference_flag, &bit_pos, 1);
    int frame_cropping_flag;
    int vui_parameters_present_flag;
    pSPS += read_bit(pSPS, &frame_cropping_flag, &bit_pos, 1);
    if (frame_cropping_flag){
        int frame_crop_left_offset;
        int frame_crop_right_offset;
        int frame_crop_top_offset;
        int frame_crop_bottom_offset;
        pSPS += parse_ue(pSPS,&frame_crop_left_offset, &bit_pos);
        pSPS += parse_ue(pSPS,&frame_crop_right_offset, &bit_pos);
        pSPS += parse_ue(pSPS,&frame_crop_top_offset, &bit_pos);
        pSPS += parse_ue(pSPS,&frame_crop_bottom_offset, &bit_pos);
    }
    pSPS += read_bit(pSPS, &vui_parameters_present_flag, &bit_pos, 1);

    pH264Info->time_base_num = 90000;
    pH264Info->time_base_den = 3003;
    if (vui_parameters_present_flag) {
        int aspect_ratio_info_present_flag;
        pSPS += read_bit(pSPS, &aspect_ratio_info_present_flag, &bit_pos, 1);
        if (aspect_ratio_info_present_flag){
            int aspect_ratio_idc;
            pSPS += read_bit(pSPS, &aspect_ratio_idc,&bit_pos,8);
            if (aspect_ratio_idc == 255) // Extended_SAR
            {
                int sar_width;
                int sar_height;
                pSPS += read_bit(pSPS, &sar_width, &bit_pos, 16);
                pSPS += read_bit(pSPS, &sar_height, &bit_pos, 16);
            }
        }
        int overscan_info_present_flag;
        pSPS += read_bit(pSPS, &overscan_info_present_flag, &bit_pos, 1);
        if (overscan_info_present_flag){
            int overscan_appropriate_flag;
            pSPS += read_bit(pSPS, &overscan_appropriate_flag, &bit_pos, 1);
        }
        int video_signal_type_present_flag;
        pSPS += read_bit(pSPS, &video_signal_type_present_flag, &bit_pos, 1);
        if (video_signal_type_present_flag){
            int video_format;
            pSPS += read_bit(pSPS, &video_format, &bit_pos,3);
            int video_full_range_flag;
            pSPS += read_bit(pSPS, &video_full_range_flag, &bit_pos, 1);
            int colour_description_present_flag;
            pSPS += read_bit(pSPS, &colour_description_present_flag, &bit_pos, 1);
            if (colour_description_present_flag){
                int colour_primaries, transfer_characteristics,matrix_coefficients;
                pSPS += read_bit(pSPS, &colour_primaries, &bit_pos, 8);
                pSPS += read_bit(pSPS, &transfer_characteristics, &bit_pos, 8);
                pSPS += read_bit(pSPS, &matrix_coefficients, &bit_pos, 8);
            }
        }

        int chroma_loc_info_present_flag;
        pSPS += read_bit(pSPS, &chroma_loc_info_present_flag, &bit_pos, 1);
        if (chroma_loc_info_present_flag) {
            int chroma_sample_loc_type_top_field;
            int chroma_sample_loc_type_bottom_field;
            pSPS += parse_ue(pSPS,&chroma_sample_loc_type_top_field, &bit_pos);
            pSPS += parse_ue(pSPS,&chroma_sample_loc_type_bottom_field, &bit_pos);
        }
        int timing_info_present_flag;
        pSPS += read_bit(pSPS, &timing_info_present_flag, &bit_pos, 1);
        if (timing_info_present_flag) {
            int num_units_in_tick,time_scale;
            int fixed_frame_rate_flag;
            pSPS += read_bit(pSPS, &num_units_in_tick, &bit_pos, 32);
            pSPS += read_bit(pSPS, &time_scale, &bit_pos, 32);
            pSPS += read_bit(pSPS, &fixed_frame_rate_flag, &bit_pos, 1);
            if (fixed_frame_rate_flag) {
                unsigned char divisor; //when pic_struct_present_flag == 1 && pic_struct == 0
                //pH264Info->fps = (float)time_scale/num_units_in_tick/divisor;
                if (frame_mbs_only_flag) {
                    divisor = 2;
                } else {
                    divisor = 1; // interlaced
                }
                pH264Info->time_base_num = time_scale;
                pH264Info->time_base_den = divisor * num_units_in_tick;
                //printf("pH264Info->fps = [%d][%d]\n", pH264Info->time_base_num,pH264Info->time_base_den);
            }
        }
    }

    if ((pH264Info->width == 0) ||(pH264Info->height == 0) ||(pH264Info->time_base_num == 0) || (pH264Info->time_base_den == 0)) {
        return -1;
    }
    return 0;
}

#define NAL_HEADER_LEN     4
#define NAL_HEADER_PREFIX(data) \
        ((data[0] == 0x00) &&   \
         (data[1] == 0x00) &&   \
         (data[2] == 0x00) &&   \
         (data[3] == 0x01))

#define NAL_SLICE_PREFIX(data) \
        ((data[0] == 0x00) &&  \
         (data[1] == 0x00) &&  \
         (data[2] == 0x01))

#define NAL_TYPE(data) \
        (data[4]&0x1F)

static int find_nalu_header_pos(uint8_t *data, int len, int start)
{
    int pos = start;
    while (pos < len - NAL_HEADER_LEN) {
        if (NAL_HEADER_PREFIX((data+pos))) {
            break;
        }
        ++pos;
    }
    if (pos == len - NAL_HEADER_LEN) {
        pos = -1;
    }
    return pos;
}

static int h264_get_extra_data(uint8_t *input, int in_len, uint8_t *extra, int *extra_len)
{
    int pos = 0;
    int nal_pos = 0;

    *extra_len = 0;
    while (pos < in_len) {
        pos = find_nalu_header_pos(input, in_len, nal_pos + 4);
        if (pos == -1 || pos >= in_len - 4) {
            pos = in_len;
        }
        int nal_len = pos - nal_pos;
        uint8_t *nal = input + nal_pos;
        switch (NAL_TYPE(nal)) {
        case H264_NAL_SPS:
            memcpy(extra, nal, nal_len);
            *extra_len = nal_len;
            break;
        case H264_NAL_PPS:
            memcpy(&extra[*extra_len], nal, nal_len);
            *extra_len += nal_len;
            return 0;
            break;
        default:
            printf("got nal_type=%d, nal=0x%X\n", NAL_TYPE(nal), nal[4]);
            break;
        }
        nal_pos = pos;
    }
    if (*extra_len == 0) {
        return -1;
    }
    return 0;
}

static void set_nal_len(uint8_t *buf, int len)
{
    *(buf + 0) = len >> 24;
    *(buf + 1) = len >> 16;
    *(buf + 2) = len >> 8;
    *(buf + 3) = len >> 0;
}

static const uint8_t *avc_find_startcode_internal(const uint8_t *p,
						     const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) {
		uint32_t x = *(const uint32_t *)p;

		if ((x - 0x01010101) & (~x) & 0x80808080) {
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}

			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out = avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
		out--;
	return out;
}

bool is_keyframe(const uint8_t *data, size_t size)
{
	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;
	int type;

	nal_start = avc_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		type = nal_start[0] & 0x1F;

		if (type == H264_NAL_IDR_SLICE || type == H264_NAL_SLICE)
			return (type == H264_NAL_IDR_SLICE);

		nal_end = avc_find_startcode(nal_start, end);
		nal_start = nal_end;
	}

	return false;
}

static int h264_parse(uint8_t *data, int len, uint8_t *out_buf, int *total_len, int *key_frame)
{
    int pos = 0,prepos;
    int nal_len,nal_type;
    uint8_t *nal;

    if (len > MAX_NALS_LEN) {
        printf("h264_parse failed len=%d\n", len);
        return -1;
    }

    *total_len = 0;
    *key_frame = 0;


    while (pos < len - 4) {
        if (NAL_HEADER_PREFIX((data+pos))) {
            break;
        }
        ++pos;
    }
    prepos = pos;

    while (1) {
        pos = prepos + 4;
        while (pos < len - 4) {
            if (NAL_HEADER_PREFIX((data+pos))) {
                break;
            }
            ++pos;
        }

        if (pos >= len -4) {
            pos = len;
            nal_len = pos - prepos - 4;
            nal = &data[prepos + 4];
            nal_type = nal[0] & 0x1F;
            if ((!*key_frame) && (nal_type == H264_NAL_IDR_SLICE)) {
                *key_frame = 1;
            }
            set_nal_len(out_buf+*total_len, nal_len);
            memcpy(out_buf+*total_len+4, nal, nal_len);
            *total_len += 4 + nal_len;
            break;
        }

        nal_len = pos - prepos - 4;
        nal = &data[prepos + 4];
        nal_type = nal[0] &  0x1F;
        if ((!*key_frame) && (nal_type == H264_NAL_IDR_SLICE)){
            *key_frame = 1;
        }
        set_nal_len(out_buf+*total_len, nal_len);
        memcpy(out_buf+*total_len + 4, nal, nal_len);
        *total_len += 4 + nal_len;

        prepos = pos;
    }
    if (!*total_len) {
        return -1;
    }
    return 0;
}

#if 0
static int gettimeofday_clock(struct timeval *tv, int *tz)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    tv->tv_sec = now.tv_sec;
    tv->tv_usec = now.tv_nsec/1000;
    return 0;
}
#endif

#if 0
static void update_timestamp(struct media_packet *pkt, struct video_packet *vp)
{
#if 0
    struct rtmpc *rtmpc = (struct rtmpc *)pkt->parent;
    struct timeval now;
    gettimeofday_clock(&now, NULL);
    unsigned long long msecs = now.tv_sec * 1000 + now.tv_usec/1000;
    unsigned long curr_msecs = (unsigned long)(msecs & 0xffffffff);
    if (!rtmpc->prev_msec) {
        rtmpc->prev_msec = curr_msecs;
        rtmpc->prev_timestamp = 0;
    }
    unsigned long diff;
    if(curr_msecs >= rtmpc->prev_msec){
       diff = curr_msecs - rtmpc->prev_msec;
    }else{
        diff = curr_msecs + (0xffffffff - rtmpc->prev_msec);
    }
    rtmpc->prev_msec = curr_msecs;

    pkt->timestamp = rtmpc->prev_timestamp + diff;
    rtmpc->prev_timestamp += diff;
#else
    pkt->video->dts = vp->dts;
    pkt->video->pts = vp->pts;
    pkt->video->timebase_num = vp->timebase_num;
    pkt->video->timebase_den = vp->timebase_den;

#endif
}
#endif

static int strip_nalu_sps_pps(uint8_t *data, int len)
{
    int got_sps = 0, got_pps = 0;
    int pos = 0;
    int i = 0, j = 0;
    int index[16] = {0};
    while (pos < len - 4) {
        if (NAL_SLICE_PREFIX((data+pos))) {
            index[i] = pos;
            ++i;
            pos += 3;
        } else if (NAL_HEADER_PREFIX((data+pos))) {
            index[i] = pos;
            ++i;
            pos += 4;
        }
        ++pos;
    }
    if (i == 0) {
        return -1;
    }
    for (j = 0; j < i; ++j) {
        pos = index[j];
        uint8_t *nal = &data[pos];
        int nal_type = nal[4] &  0x1F;
        switch (nal_type) {
        case H264_NAL_SLICE:
            break;
        case H264_NAL_IDR_SLICE:
            break;
        case H264_NAL_SEI:
            break;
        case H264_NAL_SPS:
            got_sps = 1;
            break;
        case H264_NAL_PPS:
            got_pps = 1;
            break;
        default:
            //printf("nal_type=%d\n", nal_type);
            break;
        }
        if (got_sps && got_pps) {
            return index[j+1];
        }
    }
    return 0;
}

#define MILLISECOND_DEN 1000

static int32_t get_ms_time(struct video_packet *packet, int64_t val)
{
	return (int32_t)(val * MILLISECOND_DEN / packet->timebase_den);
}

static int32_t get_ms_time2(struct video_packet *packet, int64_t val)
{
	return (int32_t)(val * MILLISECOND_DEN / packet->timebase_den);
}

int h264_send_packet(struct rtmpc *rtmpc, struct video_packet *pkt)
{
    if (pkt->size <= 4) {
        printf("H264 data length is invalid\n");
        return -1;
    }
    int total_len = 0;
    int key_frame = 0;
    int ret = 0;
    int offset = strip_nalu_sps_pps(pkt->data, pkt->size);
    if (offset) {
        pkt->data += offset;
        pkt->size -= offset;
    }

    ret = h264_parse(pkt->data, pkt->size, rtmpc->tmp_buf.iov_base, &total_len, &key_frame);
    if (ret != 0) {
        printf("h264_parse failed!\n");
        return -1;
    }
    if (!rtmpc->is_keyframe_got) {
        if (key_frame) {
            rtmpc->start_dts_offset = get_ms_time(pkt, pkt->dts);
            rtmpc->is_keyframe_got = true;
            printf("start_dts_offset=%zu, dts=%zu, timebase_den=%u\n", rtmpc->start_dts_offset, pkt->dts, pkt->timebase_den);
        } else {
            printf("wait for key frame\n");
            return 0;
        }
    }

    struct media_packet *mpkt = media_packet_create(MEDIA_PACKET_VIDEO, NULL, 0);
    mpkt->video->key_frame = key_frame;
    mpkt->video->dts = pkt->dts;
    mpkt->video->pts = pkt->pts;
    mpkt->video->timebase_num = pkt->timebase_num;
    mpkt->video->timebase_den = pkt->timebase_den;

    struct item *item = item_alloc(rtmpc->q, rtmpc->tmp_buf.iov_base, total_len, mpkt);

    if (0 != queue_push(rtmpc->q, item)) {
        printf("queue_push failed!\n");
    }
    return 0;
}

int h264_add(struct rtmpc *rtmpc, struct video_packet *pkt)
{
    int extra_len = 0;
    struct rtmp_h264_info info;

    rtmpc->video = (struct rtmp_video_params *)calloc(1, sizeof(struct rtmp_video_params));
    if (!pkt->extra_data || pkt->extra_size == 0) {
        printf("video packet no extra_data\n");
        return -1;
    }
    rtmpc->video->extra_data = calloc(1, pkt->extra_size);
    if (!rtmpc->video->extra_data) {
        printf("calloc failed\n");
        return -1;
    }
    if (-1 == h264_get_extra_data(pkt->extra_data, pkt->extra_size, rtmpc->video->extra_data, &extra_len)) {
        printf("h264_get_extra_data failed!\n");
        return -1;
    }

    if (-1 == get_h264_info(rtmpc->video->extra_data, extra_len, &info)) {
        printf("get_h264_info failed!\n");
        return -1;
    }

    rtmpc->video->extra_data_len = extra_len;
    rtmpc->video->width = info.width;
    rtmpc->video->height = info.height;
    rtmpc->video->framerate = (double)info.time_base_num * (double)1.0/(double)info.time_base_den;
    return 0;
}

int h264_write_header(struct rtmpc *rtmpc)
{
#define AV_RB32(x)                                  \
        ((((const unsigned char*)(x))[0] << 24) |   \
         (((const unsigned char*)(x))[1] << 16) |   \
         (((const unsigned char*)(x))[2] <<  8) |   \
         ((const unsigned char*)(x))[3])

    struct rtmp_private_buf *buf = rtmpc->priv_buf;
    unsigned int timestamp = 0;
    put_byte(buf, FLV_TAG_TYPE_VIDEO);
    put_be24(buf, 0); //size patched later
    put_be24(buf, timestamp); // ts
    put_byte(buf, (timestamp >> 24)&0x7f); // ts ext, timestamp is signed-32 bit
    put_be24(buf, 0); // streamid
    unsigned int pos = tell(buf);

    put_byte(buf, FLV_CODECID_H264| FLV_FRAME_KEY); // flags
    put_byte(buf, 0); // AVC sequence header
    put_be24(buf, 0); // composition time, 0 for AVC

    //parse extra_data to get sps/pps,TODO
    unsigned char *sps,*pps;
    int sps_size,pps_size;
    int startcode_len = 4;

    sps = rtmpc->video->extra_data;
    sps_size = 0;
    for (int i = 4; i < rtmpc->video->extra_data_len - 4; i++) {
        if ((AV_RB32(&sps[i]) == 0x00000001) /*|| (AV_RB24(&sps[i]) == 0x000001)*/){
            sps_size = i;
            //startcode_len = (AV_RB32(&sps[i]) == 0x00000001) ? 4:3;
            break;
        }
    }
    if (!sps_size) {
        return -1;
    }
    pps = sps + sps_size;
    pps_size = rtmpc->video->extra_data_len - sps_size;

    sps += startcode_len;
    put_byte(buf, 1);      // version
    put_byte(buf, sps[1]); // profile
    put_byte(buf, sps[2]); // profile
    put_byte(buf, sps[3]); // level

    //lengthSizeMinusOne = ff -- FLV NALU-length bytes = (lengthSizeMinusOne & 3）+1 = 4
    put_byte(buf, 0xff);
    //numOfSequenceParameterSets = E1 -- (nummOfSequenceParameterSets & 0x1F) == 1, one sps exists
    put_byte(buf, 0xe1);
    put_be16(buf, sps_size - startcode_len);
    append_data(buf, sps, sps_size - startcode_len);

    // fill PPS
    put_byte(buf, 1); // number of pps
    put_be16(buf, pps_size - startcode_len);
    append_data(buf, pps + startcode_len, pps_size - startcode_len );

    unsigned int data_size = tell(buf) - pos;
    update_amf_be24(buf, data_size,pos - 10);
    put_be32(buf, data_size + 11); // Last tag size
    return 0;
}

int h264_write_packet(struct rtmpc *rtmpc, struct video_packet *pkt)
{
    struct rtmp_private_buf *buf = rtmpc->priv_buf;

	int64_t offset = pkt->pts - pkt->dts;
	int32_t time_ms = get_ms_time2(pkt, pkt->dts) - rtmpc->start_dts_offset;

    put_byte(buf, FLV_TAG_TYPE_VIDEO);
    put_be24(buf, pkt->size + 5);
    put_be24(buf, time_ms); //timestamp is dts
    put_byte(buf, (time_ms >> 24)&0x7f);// timestamps are 32bits _signed_
    put_be24(buf, 0);//streamId, always 0

    int tag_flags = FLV_CODECID_H264 | (pkt->key_frame ? FLV_FRAME_KEY: FLV_FRAME_INTER);
    put_byte(buf, tag_flags);
    put_byte(buf, 1);// AVC NALU
    put_be24(buf, get_ms_time2(pkt, offset));// composition time offset, TODO, pts - dts
    append_data(buf, pkt->data, pkt->size);
    put_be32(buf, 11 + pkt->size + 5);
    if (flush_data(rtmpc, 1)) {
        return -1;
    }
    return 0;
}
