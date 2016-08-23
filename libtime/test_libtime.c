/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libtime.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-05-21 17:28:20
 * updated: 2016-05-21 17:28:20
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "libtime.h"

void foo()
{
    char time[32];
    printf("time_get_sec_str:     %s", time_get_sec_str());
    printf("time_get_msec_str:    %s\n", time_get_msec_str(time, sizeof(time)));
    printf("time_get_sec:         %" PRIu32 "\n", time_get_sec());
    printf("time_get_msec:        %" PRIu64 "\n", time_get_msec());
    printf("time_get_msec:        %" PRIu64 "\n", time_get_usec()/1000);
    printf("time_get_usec:        %" PRIu64 "\n", time_get_usec());
    printf("time_get_nsec:        %" PRIu64 "\n", time_get_nsec());
    printf("time_get_nsec_bootup: %" PRIu64 "\n", time_get_nsec_bootup());
}

int main(int argc, char **argv)
{
    foo();
    time_sleep_ms(1000);
    foo();
    return 0;
}
