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
#define __STDC_FORMAT_MACROS
#if defined (__linux__) || defined (__CYGWIN__)
#include <inttypes.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif
#include "libtime.h"

void foo()
{
    char time[32];
    printf("time_sec_str:     %s", time_sec_str());
    printf("time_msec_str:    %s\n", time_msec_str(time, sizeof(time)));
    printf("time_sec:         %" PRIu64 "\n", time_sec());
    printf("time_msec:        %" PRIu64 "\n", time_msec());
    printf("time_msec:        %" PRIu64 "\n", time_usec(NULL)/1000);
    printf("time_usec:        %" PRIu64 "\n", time_usec(NULL));
    printf("time_nsec:        %" PRIu64 "\n", time_nsec());
    printf("time_nsec_bootup: %" PRIu64 "\n", time_nsec_bootup());
}

int main(int argc, char **argv)
{
    foo();
    time_sleep_ms(1000);
    foo();
    return 0;
}
