/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libtime.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-05-21 17:28:20
 * updated: 2016-05-21 17:28:20
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/timeb.h>

#include "libtime.h"

uint32_t time_get_sec()
{
    time_t t;
    t = time(NULL);
    if (t == -1) {
        printf("time failed %d:%s\n", errno, strerror(errno));
    }
    return t;
}

char *time_get_sec_str()
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

uint64_t time_get_usec()
{
    struct timeval tv;
    if (-1 == gettimeofday(&tv, NULL)) {
        printf("gettimeofday failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
    return (tv.tv_sec*1000*1000 + tv.tv_usec);
}

uint64_t _time_clock_gettime(clockid_t clk_id)
{
    struct timespec ts;
    if (-1 == clock_gettime(clk_id, &ts)) {
        printf("clock_gettime failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
    return ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
}

uint64_t time_get_msec()
{
    struct timeb tb;
    ftime(&tb);
    return tb.time * 1000 + tb.millitm;
}

uint64_t time_get_nsec()
{
    return _time_clock_gettime(CLOCK_REALTIME);
}

uint64_t time_get_nsec_bootup()
{
    return _time_clock_gettime(CLOCK_MONOTONIC);
}

char *time_get_msec_str(char *str, int len)
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
    if (NULL == localtime_r(&now_sec, &now_tm)) {
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
