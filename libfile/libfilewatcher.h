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
#ifndef LIBFILEWATCHER_H
#define LIBFILEWATCHER_H

#include <dirent.h>
#include <libdict.h>
#include <libgevent.h>

#ifdef __cplusplus
extern "C" {
#endif

enum fw_type {
    FW_CREATE_DIR = 0,
    FW_CREATE_FILE,
    FW_DELETE_DIR,
    FW_DELETE_FILE,
    FW_MOVE_FROM_DIR,
    FW_MOVE_TO_DIR,
    FW_MOVE_FROM_FILE,
    FW_MOVE_TO_FILE,
    FW_MODIFY_FILE,
};

typedef struct fw {
    int fd;
    struct gevent_base *evbase;
    dict *dict_path;
    void (*notify_cb)(struct fw *fw, enum fw_type type, char *path);
} fw_t;


struct fw *fw_init(void (notify_cb)(struct fw *fw, enum fw_type type, char *path));
void fw_deinit(struct fw *fw);
int fw_add_watch_recursive(struct fw *fw, const char *path);
int fw_del_watch_recursive(struct fw *fw, const char *path);
int fw_dispatch(struct fw *fw);


#ifdef __cplusplus
}
#endif
#endif
