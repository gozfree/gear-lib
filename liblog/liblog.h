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
#ifndef LIBLOG_H
#define LIBLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define	LOG_EMERG   0  /* system is unusable */
#define	LOG_ALERT   1  /* action must be taken immediately */
#define	LOG_CRIT    2  /* critical conditions */
#define	LOG_ERR     3  /* error conditions */
#define	LOG_WARNING 4  /* warning conditions */
#define	LOG_NOTICE  5  /* normal but significant condition */
#define	LOG_INFO    6  /* informational */
#define	LOG_DEBUG   7  /* debug-level messages */
#define	LOG_VERB    8  /* verbose messages */


typedef enum {
    LOG_STDIN   = 0, /*stdin*/
    LOG_STDOUT  = 1, /*stdout*/
    LOG_STDERR  = 2, /*stderr*/
    LOG_FILE    = 3,
    LOG_RSYSLOG = 4,

    LOG_MAX_OUTPUT = 255
} log_type_t;

int log_init(int type, const char *ident);
void log_deinit();

void log_set_level(int level);
void log_set_split_size(int size);
void log_set_rotate(int enable);
int log_set_path(const char *path);
int log_print(int lvl, const char *tag, const char *file, int line,
        const char *func, const char *fmt, ...);

#define LOG_LEVEL_ENV     "LIBLOG_LEVEL"
#define LOG_OUTPUT_ENV    "LIBLOG_OUTPUT"
#define LOG_TIMESTAMP_ENV "LIBLOG_TIMESTAMP"

#define LOG_TAG "tag"

#define loge(...) log_print(LOG_ERR, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logw(...) log_print(LOG_WARNING, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logi(...) log_print(LOG_INFO, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logd(...) log_print(LOG_DEBUG, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logv(...) log_print(LOG_VERB, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
