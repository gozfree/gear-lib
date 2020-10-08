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
#include "libuac.h"
#include <gear-lib/libfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define OUTPUT_FILE     "uac.pcm"

static struct file *fp;

static int on_frame(struct uac_ctx *c, struct audio_frame *frm)
{
    printf("frm[%" PRIu64 "] cnt=%d size=%" PRIu64 ", ts=%" PRIu64 " ms\n",
           frm->frame_id, frm->frames, frm->total_size, frm->timestamp/1000000);
    file_write(fp, frm->data[0], frm->total_size);
    return 0;
}

static int foo()
{
    struct uac_config conf = {
        .format = SAMPLE_FORMAT_PCM_S16LE,
        .sample_rate = 44100,
        .channels = 2,
    };
    struct uac_ctx *uac = uac_open("xxx", &conf);
    if (!uac) {
        printf("uac_open failed!\n");
        return -1;
    }
    printf("sample_rate: %d\n", uac->conf.sample_rate);
    printf("    channel: %d\n", uac->conf.channels);
    printf("     format: %s\n", sample_format_to_string(uac->conf.format));
    printf("     device: %s\n", uac->conf.device);
    fp = file_open(OUTPUT_FILE, F_CREATE);
    uac_start_stream(uac, on_frame);
    sleep(5);
    uac_stop_stream(uac);
    file_close(fp);
    uac_close(uac);
    printf("write %s fininshed!\n", OUTPUT_FILE);

    return 0;
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
