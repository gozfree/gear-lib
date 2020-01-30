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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <sys/time.h>
#include <sys/timeb.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif

#include "libtime.h"

#define TIME_FORMAT "%Y%m%d%H%M%S"


uint64_t time_sec()
{
    time_t t;
    t = time(NULL);
    if (t == -1) {
        printf("time failed %d:%s\n", errno, strerror(errno));
    }
    return t;
}

char *time_sec_str()
{
    time_t t;
    struct tm *tm;
    t = time(NULL);
    if (t == -1) {
        printf("time failed %d:%s\n", errno, strerror(errno));
    }
    tm = localtime(&t);
    if (!tm) {
        printf("localtime failed %d:%s\n", errno, strerror(errno));
    }
    return asctime(tm);
}

char *time_str_human(char *str, int len)
{
    struct time_info ti;
    if (-1 == time_info(&ti)) {
        return NULL;
    }
    snprintf(str, len, "%04d%02d%02d%02d%02d%02d",
             ti.year, ti.mon, ti.day, ti.hour, ti.min, ti.sec);
    return str;
}

char *time_str_human_by_utc(uint32_t utc, char *str, int len)
{
    struct time_info ti;
    if (-1 == time_info_by_utc(utc, &ti)) {
        return NULL;
    }
    snprintf(str, len, "%04d%02d%02d%02d%02d%02d",
             ti.year, ti.mon, ti.day, ti.hour, ti.min, ti.sec);
    return str;
}

char *time_str_human_by_msec(uint64_t msec, char *str, int len)
{
    struct time_info ti;
    if (-1 == time_info_by_msec(msec, &ti)) {
        return NULL;
    }
    snprintf(str, len, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             ti.year, ti.mon, ti.day, ti.hour, ti.min, ti.sec, ti.msec);
    return str;
}


char *time_str_human_by_timeval(struct timeval *tv, char *str, int len)
{
    uint32_t sec = time_usec(tv)/1000000;
    return time_str_human_by_utc(sec, str, len);
}

uint64_t time_usec(struct timeval *val)
{
    struct timeval tv;
    if (val == NULL) {
        val = &tv;
    }
    if (-1 == gettimeofday(val, NULL)) {
        printf("gettimeofday failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
    return (uint64_t)(((uint64_t)tv.tv_sec)*1000*1000 + (uint64_t)tv.tv_usec);
}

uint64_t _time_clock_gettime(clockid_t clk_id)
{
#if defined (__linux__) || defined (__CYGWIN__)
    struct timespec ts;
    if (-1 == clock_gettime(clk_id, &ts)) {
        printf("clock_gettime failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
    return (uint64_t)(((uint64_t)ts.tv_sec*1000*1000*1000) + (uint64_t)ts.tv_nsec);
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    return 0;
#endif
}

uint64_t time_msec()
{
#if defined (__linux__) || defined (__CYGWIN__)
    struct timeb tb;
    ftime(&tb);
    return (uint64_t)(((uint64_t)tb.time) * 1000 + (uint64_t)tb.millitm);
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    return 0;
#endif
}

uint64_t time_nsec()
{
    return _time_clock_gettime(CLOCK_REALTIME);
}

uint64_t time_nsec_bootup()
{
    return _time_clock_gettime(CLOCK_MONOTONIC);
}

char *time_msec_str(char *str, int len)
{
    char date_fmt[20];
    char date_ms[4];
    struct timeval tv;
    struct tm now_tm;
    int now_ms;
    time_t now_sec;
    if (len < 24) {
        printf("time string len must bigger than 24\n");
        return NULL;
    }
    if (-1 == gettimeofday(&tv, NULL)) {
        printf("gettimeofday failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec/1000;
    if (NULL != localtime_r(&now_sec, &now_tm)) {
        printf("localtime_r failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }

    strftime(date_fmt, 20, "%Y-%m-%d %H:%M:%S", &now_tm);
    snprintf(date_ms, sizeof(date_ms), "%03d", now_ms);
    snprintf(str, len, "%s.%s", date_fmt, date_ms);
    return str;
}

int time_sleep_ms(uint64_t ms)
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ms*1000;
    return select(0, NULL, NULL, NULL, &tv);
}

int time_info_by_utc(uint32_t utc, struct time_info *ti)
{
    struct timeval tv;
    struct timezone tz;
    struct tm *now;
    char date_fmt[20] = {0};
    char date_ms[8] = {0};

    now = localtime((time_t *)&utc);
    if (!now) {
        printf("localtime failed: %d\n", errno);
        return -1;
    }
    if (-1 == gettimeofday(&tv, &tz)) {
        printf("gettimeofday failed: %d\n", errno);
        return -1;
    }

    ti->utc = (uint32_t)utc;
    ti->year = now->tm_year + 1900;
    ti->mon = now->tm_mon + 1;
    ti->day = now->tm_mday;
    ti->hour = now->tm_hour;
    ti->min = now->tm_min;
    ti->sec = now->tm_sec;
    ti->timezone = (-tz.tz_minuteswest) / 60;

    strftime(date_fmt, sizeof(ti->str), TIME_FORMAT, now);
    snprintf(date_ms, sizeof(date_ms), "%03d", ti->msec);
    snprintf(ti->str, sizeof(ti->str), "%s%s", date_fmt, date_ms);

    return 0;
}

int time_info_by_msec(uint64_t msec, struct time_info *ti)
{
    struct timeval tv;
    struct timezone tz;
    struct tm *now;
    char date_fmt[20] = {0};
    char date_ms[8] = {0};
    uint32_t utc;
    uint16_t dot_msec;

    utc = (uint32_t)(msec/1000);
    dot_msec = (msec - utc*1000);
    now = localtime((time_t *)&utc);
    if (!now) {
        printf("localtime failed: %d\n", errno);
        return -1;
    }
    if (-1 == gettimeofday(&tv, &tz)) {
        printf("gettimeofday failed: %d\n", errno);
        return -1;
    }

    ti->utc = utc;
    ti->utc_msec = msec;
    ti->year = now->tm_year + 1900;
    ti->mon = now->tm_mon + 1;
    ti->day = now->tm_mday;
    ti->hour = now->tm_hour;
    ti->min = now->tm_min;
    ti->sec = now->tm_sec;
    ti->msec = dot_msec;
    ti->timezone = (-tz.tz_minuteswest) / 60;

    strftime(date_fmt, sizeof(ti->str), TIME_FORMAT, now);
    snprintf(date_ms, sizeof(date_ms), "%03d", ti->msec);
    snprintf(ti->str, sizeof(ti->str), "%s%s", date_fmt, date_ms);

    return 0;
}

int time_info(struct time_info *ti)
{
    time_t utc;
    struct timeval tv;
    struct timezone tz;
    struct tm *now;
    char date_fmt[20] = {0};
    char date_ms[8] = {0};

    if (-1 == time(&utc)) {
        return -1;
    }
    if (-1 == gettimeofday(&tv, &tz)) {
        return -1;
    }
    now = localtime(&utc);
    if (!now) {
        return -1;
    }

    ti->utc = (uint32_t)utc;
    ti->utc_msec = ((uint64_t)utc)*1000 + ti->msec;
    ti->year = now->tm_year + 1900;
    ti->mon = now->tm_mon + 1;
    ti->day = now->tm_mday;
    ti->hour = now->tm_hour;
    ti->min = now->tm_min;
    ti->sec = now->tm_sec;
    ti->msec = tv.tv_usec/1000;
    ti->timezone = (-tz.tz_minuteswest) / 60;

    strftime(date_fmt, sizeof(ti->str), TIME_FORMAT, now);
    snprintf(date_ms, sizeof(date_ms), "%03d", ti->msec);
    snprintf(ti->str, sizeof(ti->str), "%s%s", date_fmt, date_ms);

    return 0;
}

#if defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#else
int time_set_info(struct time_info *ti)
{
    time_t timep;
    struct tm tm;
    struct timeval tv, tv1;
    struct timezone tz;
    tm.tm_sec = ti->sec;
    tm.tm_min = ti->min;
    tm.tm_hour = ti->hour;
    tm.tm_mday = ti->day;
    tm.tm_mon = ti->mon - 1;
    tm.tm_year = ti->year - 1900;

    timep = mktime(&tm);
    tv.tv_sec = timep;
    tv.tv_usec = 0;

    gettimeofday(&tv1, &tz);
    tz.tz_minuteswest = -(ti->timezone) * 60;
    if (-1 == settimeofday(&tv, &tz)) {
        return -1;
    }
    return 0;
}
#endif

bool time_passed_sec(int sec)
{
    bool ret = false;
    static uint64_t last_sec = 0;
    static uint64_t now_sec = 0;
    now_sec = time_sec();
    if (last_sec == 0) {
        last_sec = now_sec;
    }
    if (now_sec - last_sec >= (uint32_t)sec) {
        ret = true;
        last_sec = now_sec;
    } else {
        ret = false;
    }
    return ret;
}
