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
#include <libposix.h>
#include "libtime.h"
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

void foo()
{
    struct time_info ti;
    char ts[64];
    char time[32];
    printf("time_now_sec_str:     %s", time_now_sec_str());
    printf("time_now_msec_str:    %s\n", time_now_msec_str(time, sizeof(time)));
    printf("time_now_sec:         %" PRIu64 "\n", time_now_sec());
    printf("time_now_msec:        %" PRIu64 "\n", time_now_msec());
    printf("time_now_msec:        %" PRIu64 "\n", time_now_usec(NULL)/1000);
    printf("time_now_usec:        %" PRIu64 "\n", time_now_usec(NULL));
    printf("time_now_nsec:        %" PRIu64 "\n", time_now_nsec());
    printf("time_bootup_nsec:     %" PRIu64 "\n", time_bootup_nsec());
    time_now_info(&ti);
    printf("time_now_info:        %d-%02d-%02d %02d:%02d:%02d.%03d\n",
                    ti.year, ti.mon, ti.day, ti.hour, ti.min, ti.sec, ti.msec);
    printf("time_now_info.str:    %s\n", ti.str);
    memset(ts, 0, sizeof(ts));
    printf("time_str_by_msec:     %s\n", time_str_format_by_msec(ti.utc_msec, ts, sizeof(ts)));
}

int main(int argc, char **argv)
{
    foo();
    time_sleep_ms(1000);
    foo();
    return 0;
}
