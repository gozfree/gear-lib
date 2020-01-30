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
#ifndef LIBTIME_H
#define LIBTIME_H

#include <stdint.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <stdbool.h>
#endif
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
uint64_t time_sec();
char *time_sec_str();
char *time_str_human(char *str, int len);
char *time_str_human_by_utc(uint32_t utc, char *str, int len);
char *time_str_human_by_msec(uint64_t msec, char *str, int len);
char *time_str_human_by_timeval(struct timeval *val, char *str, int len);

/*
 * accuracy milli second
 */
uint64_t time_msec();
char *time_msec_str(char *str, int len);
int time_sleep_ms(uint64_t ms);

/*
 * accuracy micro second
 */
uint64_t time_usec(struct timeval *tv);

/*
 * accuracy nano second
 */
uint64_t time_nsec();
uint64_t time_nsec_bootup();
char *time_nsec_to_str(uint64_t nsec);

int time_info(struct time_info *ti);
int time_info_by_utc(uint32_t utc, struct time_info *ti);
int time_info_by_msec(uint64_t msec, struct time_info *ti);

bool time_passed_sec(int sec);

#ifdef __cplusplus
}
#endif
#endif
