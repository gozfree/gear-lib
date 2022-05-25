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
#include <wchar.h>
#include <errno.h>
#include <stdarg.h>
#include <windows.h>
#include <unistd.h>
#include <sys/types.h>
#define HAVE_WINRT 1
#define _CRT_SECURE_NO_WARNINGS

int utf8towchar(const char *filename_utf8, wchar_t **filename_w)
{
    int num_chars;
    num_chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename_utf8, -1, NULL, 0);
    if (num_chars <= 0) {
        *filename_w = NULL;
        return 0;
    }
    *filename_w = (wchar_t *)calloc(num_chars, sizeof(wchar_t));
    if (!*filename_w) {
        errno = ENOMEM;
        return -1;
    }
    MultiByteToWideChar(CP_UTF8, 0, filename_utf8, -1, *filename_w, num_chars);
    return 0;
}

HMODULE win32_dlopen(const char *name)
{
#if _WIN32_WINNT < 0x0602
    // Need to check if KB2533623 is available
    if (!GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetDefaultDllDirectories")) {
        HMODULE module = NULL;
        wchar_t *path = NULL, *name_w = NULL;
        DWORD pathlen;
        if (utf8towchar(name, &name_w))
            goto exit;
        path = (wchar_t *)calloc(MAX_PATH, sizeof(wchar_t));
        // Try local directory first
        pathlen = GetModuleFileNameW(NULL, path, MAX_PATH);
        pathlen = wcsrchr(path, '\\') - path;
        if (pathlen == 0 || pathlen + wcslen(name_w) + 2 > MAX_PATH)
            goto exit;
        path[pathlen] = '\\';
        wcscpy_s(path + pathlen + 1, MAX_PATH, name_w);
        module = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (module == NULL) {
            // Next try System32 directory
            pathlen = GetSystemDirectoryW(path, MAX_PATH);
            if (pathlen == 0 || pathlen + wcslen(name_w) + 2 > MAX_PATH)
                goto exit;
            path[pathlen] = '\\';
            wcscpy_s(path + pathlen + 1, MAX_PATH, name_w);
            module = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        }
exit:
        free(path);
        free(name_w);
        return module;
    }
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#   define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#   define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif
#if 0 //HAVE_WINRT
    wchar_t *name_w = NULL;
    int ret;
    if (utf8towchar(name, &name_w))
        return NULL;
    ret = LoadPackagedLibrary(name_w, 0);
    free(name_w);
    return ret;
#else
    return LoadLibraryExA(name, NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
}

void showProcessInformation()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
               printf("pid %ld %s\n", pe32.th32ProcessID, pe32.szExeFile);
            } while(Process32Next(hSnapshot, &pe32));
         }
         CloseHandle(hSnapshot);
    }
}

int get_proc_name(char *name, size_t len)
{
    PROCESSENTRY32 pe32;
    int got = 0;
    HANDLE hd = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!hd) {
        return -1;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hd, &pe32)) {
        do {
            if (pe32.th32ProcessID == GetCurrentProcessId()) {
                got = 1;
                strncpy_s(name, len, pe32.szExeFile, len);
                break;
            }
        } while(Process32Next(hd, &pe32));
    }
    CloseHandle(hd);
    if (got) {
        return 0;
    } else {
        return -1;
    }
}


#define SECS_TO_FT_MULT 10000000
static LARGE_INTEGER base_time;


// Find 1st Jan 1970 as a FILETIME
static void get_base_time(LARGE_INTEGER *base_time)
{
    SYSTEMTIME st;
    FILETIME ft;

    memset(&st,0,sizeof(st));
    st.wYear=1970;
    st.wMonth=1;
    st.wDay=1;
    SystemTimeToFileTime(&st, &ft);
    base_time->LowPart = ft.dwLowDateTime;
    base_time->HighPart = ft.dwHighDateTime;
    base_time->QuadPart /= SECS_TO_FT_MULT;
}

void usleep(unsigned long usec)
{
    HANDLE timer;
    LARGE_INTEGER interval;
    interval.QuadPart = -(10 * (long)usec);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &interval, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

int win_time(struct timeval *tv, struct win_time_t *time)
{
    LARGE_INTEGER li;
    FILETIME ft;
    SYSTEMTIME st;

    li.QuadPart = tv->tv_sec;
    li.QuadPart += base_time.QuadPart;
    li.QuadPart *= SECS_TO_FT_MULT;

    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;
    FileTimeToSystemTime(&ft, &st);

    time->year = st.wYear;
    time->mon = st.wMonth-1;
    time->day = st.wDay;
    time->wday = st.wDayOfWeek;

    time->hour = st.wHour;
    time->min = st.wMinute;
    time->sec = st.wSecond;
    time->msec = tv->tv_usec/1000;

    return 0;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    int i;
    ssize_t len = 0;
    for (i = 0; i < iovcnt; ++i) {
        len += write(fd, iov[i].iov_base, iov[i].iov_len);
    }
    return len;
}

char *dup_wchar_to_utf8(wchar_t *w)
{
    char *s = NULL;
    int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
    s = malloc(l);
    if (s)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
    return s;
}

int pipe(int fds[2])
{
    HANDLE rd;
    HANDLE wr;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        return -1;
    }
    fds[0] = (int)rd;
    fds[1] = (int)wr;
    return 0;
}

void pipe_close(int fds[2])
{
    HANDLE rd = (HANDLE)fds[0];
    HANDLE wr = (HANDLE)fds[1];
    CloseHandle(rd);
    CloseHandle(wr);
}

int eventfd(unsigned int initval, int flags)
{
    HANDLE *fds;
    HANDLE rd;
    HANDLE wr;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        printf("CreatePipe failed!\n");
        return -1;
    }
    fds = calloc(2, sizeof(HANDLE));
	if (!fds) {
        printf("malloc HANDLE failed!\n");
        return -1;
    }
    fds[0] = rd;
    fds[1] = wr;
    return (int)fds;
}

void eventfd_close(int fd)
{
    HANDLE *fds = (HANDLE *)fd;
    if (!fd)
        return;
    CloseHandle(fds[0]);
    CloseHandle(fds[1]);
}

int pipe_write(int fd, const void *buf, size_t len)
{
    int written;
    if (WriteFile((HANDLE)fd, buf, len, &written, NULL)) {
        return written;
    }
    return -1;
}

int pipe_read(int fd, void *buf, size_t len)
{
    int readen;
    if (ReadFile((HANDLE)fd, buf, len, &readen, NULL)) {
        return readen;
    }
    return -1;
}

int get_nprocs()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

void *align_malloc(size_t size, size_t align)
{
    size_t  diff;
    void *ptr = malloc(size + align);
    if (ptr) {
        diff = ((~(size_t)ptr) & (align - 1)) + 1;
        ptr = (char *)ptr + diff;
        ((char *)ptr)[-1] = (char)diff;
    }
    return ptr;
}

void align_free(void *ptr)
{
    if (ptr)
        free((char *)ptr - ((char *)ptr)[-1]);
}

static inline bool has_utf8_bom(const char *in_char)
{
    uint8_t *in = (uint8_t *)in_char;
    return (in && in[0] == 0xef && in[1] == 0xbb && in[2] == 0xbf);
}

size_t utf8_to_wchar(const char *in, size_t insize, wchar_t *out, size_t outsize, int flags)
{
    int i_insize = (int)insize;
    int ret;

    if (i_insize == 0)
        i_insize = (int)strlen(in);

    /* prevent bom from being used in the string */
    if (has_utf8_bom(in)) {
        if (i_insize >= 3) {
            in += 3;
            i_insize -= 3;
        }
    }

    ret = MultiByteToWideChar(CP_UTF8, 0, in, i_insize, out, (int)outsize);

    UNUSED_PARAMETER(flags);
    return (ret > 0) ? (size_t)ret : 0;
}

size_t wchar_to_utf8(const wchar_t *in, size_t insize, char *out, size_t outsize, int flags)
{
    int i_insize = (int)insize;
    int ret;

    if (i_insize == 0)
        i_insize = (int)wcslen(in);

    ret = WideCharToMultiByte(CP_UTF8, 0, in, i_insize, out, (int)outsize, NULL, NULL);

    UNUSED_PARAMETER(flags);
    return (ret > 0) ? (size_t)ret : 0;
}

struct timerfd {
    HANDLE event;
    HANDLE timer;
};

int timerfd_create(int clockid, int flags)
{
    struct timerfd *tfd = NULL;
    HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (event == NULL) {
        printf("CreateEvent failed!\n");
        return -1;
    }
    tfd = calloc(1, sizeof(struct timerfd));
    if (!tfd) {
        printf("malloc timerfd failed!\n");
        return -1;
    }
    tfd->event = event;
    return (int)tfd;
}

static void CALLBACK timer_cb(PVOID arg, BOOLEAN TimerOrWaitFired)
{
    struct timerfd *tfd;
    if (!arg) {
        printf("timer_cb arg is invalid!\n");
        return;
    }
    tfd = (struct timerfd *)arg;
    SetEvent(tfd->event);
}

int timerfd_settime(int fd, int flags,
                    const struct itimerspec *new_value,
                    struct itimerspec *old_value)
{
    struct timerfd *tfd = (struct timerfd *)fd;
    HANDLE timer = NULL;
    DWORD ms = 0;

    if (!CreateTimerQueueTimer(&timer, NULL, timer_cb, tfd, ms, 0, WT_EXECUTEDEFAULT)) {
        printf("CreateTimerQueueTimer failed!\n");
        return -1;
    }
    tfd->timer = timer;
    return 0;
}

int timerfd_gettime(int fd, struct itimerspec *curr_value)
{
    return 0;
}
