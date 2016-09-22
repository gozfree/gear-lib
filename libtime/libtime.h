/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libtime.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-05-21 17:28:20
 * updated: 2016-05-21 17:28:20
 *****************************************************************************/
#ifndef LIBTIME_H
#define LIBTIME_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * accuracy second
 */
uint32_t time_get_sec();
char *time_get_sec_str();

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


#ifdef __cplusplus
}
#endif
#endif
