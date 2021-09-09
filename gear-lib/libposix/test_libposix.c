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
#include "libposix.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#if defined (OS_LINUX) || defined (OS_APPLE)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

static void *thread_func(void *arg)
{
    printf("this is thread\n");
    return NULL;
}

void foo()
{
    pthread_t tid;
    char proc[128] = {0};
    const char *file = "version.sh";
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (0 > stat(file, &st)) {
        printf("stat %s failed\n", file);
    }
    printf("%s size=%" PRIu32 "\n", file, (uint32_t)st.st_size);
    get_proc_name(proc, sizeof(proc));
    printf("proc name = %s\n", proc);

    pthread_create(&tid, NULL, thread_func, NULL);
    sleep(1);
    pthread_join(tid, NULL);

}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
