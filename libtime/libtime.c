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

#include "libtime.h"

int time_get_string(char *str, int len)
{
    char date_fmt[20];
    char date_ms[4];
    struct timeval tv;
    struct tm now_tm;
    int now_ms;
    time_t now_sec;
    if (len < 24) {
        printf("time string len must bigger than 24\n");
        return -1;
    }
    if (-1 == gettimeofday(&tv, NULL)) {
        printf("gettimeofday failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec/1000;
    if (NULL == localtime_r(&now_sec, &now_tm)) {
        printf("localtime_r failed %d:%s\n", errno, strerror(errno));
        return -1;
    }

    strftime(date_fmt, 20, "%Y-%m-%d %H:%M:%S", &now_tm);
    snprintf(date_ms, sizeof(date_ms), "%03d", now_ms);
    snprintf(str, len, "%s.%s", date_fmt, date_ms);
    return 0;
}
