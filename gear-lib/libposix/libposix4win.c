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
#include <errno.h>
#include <stdarg.h>
#include <windows.h>
#include <unistd.h>
#include <sys/types.h>
#define HAVE_WINRT 1

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
	printf("%s rd=%p, wr=%p\n", __func__, rd, wr);
    fds[0] = (int)rd;
    fds[1] = (int)wr;
    return 0;
}

int eventfd(unsigned int initval, int flags)
{
    int fds[2];
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
