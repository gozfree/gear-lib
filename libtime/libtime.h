/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef LIBTIME_H
#define LIBTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct time_info {
    uint64_t utc_msec;
    uint32_t utc;
    uint16_t year;
    uint8_t  mon;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint16_t msec;
    int8_t   timezone;
    char     str[32];
};

/*
 * accuracy second
 */
uint32_t time_get_sec();
char *time_get_sec_str();
char *time_get_str_human(char *str, int len);
char *time_get_str_human_by_utc(uint32_t utc, char *str, int len);

/*
 * accuracy milli second
 */
uint64_t time_get_msec();
char *time_get_msec_str(char *str, int len);
int time_sleep_ms(uint64_t ms);

/*
 * accuracy micro second
 */
uint64_t time_get_usec();

/*
 * accuracy nano second
 */
uint64_t time_get_nsec();
uint64_t time_get_nsec_bootup();

int time_get_info(struct time_info *ti);
int time_get_info_by_utc(uint32_t utc, struct time_info *ti);

bool time_passed_sec(int sec);

#ifdef __cplusplus
}
#endif
#endif
