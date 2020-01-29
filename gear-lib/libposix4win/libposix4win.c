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

#include "libposix4win.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <windows.h>
#define HAVE_WINRT 1

int access(const char *file, int mode)
{
    int ret = 0;
    WIN32_FIND_DATA info;
    HANDLE hd;
    hd = FindFirstFile(file, &info);
    if (hd != INVALID_HANDLE_VALUE) {
        ret = 0;
    } else {
        ret = -1;
    }
    FindClose(hd);
    return ret;
}

static unsigned __stdcall __thread_func(void *arg)
{
    pthread_t *h = (pthread_t *)arg;
    h->ret = h->func(h->arg);
    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg)
{
    thread->func   = start_routine;
    thread->arg    = arg;
#if HAVE_WINRT //WINRT
    thread->handle = (void*)CreateThread(NULL, 0, __thread_func, thread,
                                           0, NULL);
#else //CRT
    thread->handle = (void*)_beginthreadex(NULL, 0, __thread_func, thread,
                                           0, NULL);
#endif
    return !thread->handle;
}

int pthread_join(pthread_t thread, void **retval)
{
    DWORD ret = WaitForSingleObject(thread.handle, INFINITE);
    if (ret != WAIT_OBJECT_0) {
        if (ret == WAIT_ABANDONED)
            return EINVAL;
        else
            return EDEADLK;
    }
    if (retval)
        *retval = thread.ret;
    CloseHandle(thread.handle);
    return 0;
}

int pthread_attr_init(pthread_attr_t *attr)
{
    return 0;
}

void pthread_attr_destroy(pthread_attr_t *attr)
{
}

int pthread_mutex_init(pthread_mutex_t *lock, void *attr)
{
    *lock = CreateMutex(attr, false, NULL);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *m)
{
    CloseHandle(*m);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *m)
{
    WaitForSingleObject(*m, INFINITE);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *m)
{
    ReleaseMutex(m);
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *m, void* attr)
{
    InitializeSRWLock(m);
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *m)
{
    /* Unlocked SWR locks use no resources */
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *m)
{
    AcquireSRWLockExclusive(m);
    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *m)
{
    ReleaseSRWLockExclusive(m);
    return 0;
}

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    *sem = CreateSemaphore(NULL, 1, 1, NULL);
    return 0;
}

int sem_destroy(sem_t *sem)
{
    CloseHandle(*sem);
    return 0;
}

int sem_wait(sem_t *sem)
{
    WaitForSingleObject(*sem, INFINITE);
    return 0;
}

int sem_post(sem_t *sem)
{
    ReleaseSemaphore(*sem, 1, (LPLONG)1);
    return 0;
}


int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    BOOL pending = FALSE;
    InitOnceBeginInitialize(once_control, 0, &pending, NULL);
    if (pending)
        init_routine();
    InitOnceComplete(once_control, 0, NULL);
    return 0;
}

int pthread_cond_init(pthread_cond_t *cond, const void *unused_attr)
{
    InitializeConditionVariable(cond);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    WakeAllConditionVariable(cond);
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    SleepConditionVariableSRW(cond, (PSRWLOCK)mutex, INFINITE, 0);
    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int ms)
{
    SleepConditionVariableSRW(cond, (PSRWLOCK)mutex, ms, 0);
    return 0;
}


int pthread_cond_signal(pthread_cond_t *cond)
{
    WakeConditionVariable(cond);
    return 0;
}


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
               printf("pid %d %s\n", pe32.th32ProcessID, pe32.szExeFile);
            } while(Process32Next(hSnapshot, &pe32));
         }
         CloseHandle(hSnapshot);
    }
}

int get_proc_name(char *name, size_t len)
{
    PROCESSENTRY32 pe32;
    int got = 0;
    int i = 0;
    HANDLE hd = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!hd) {
        return -1;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hd, &pe32)) {
        do {
            if (pe32.th32ProcessID == GetCurrentProcessId()) {
                got = 1;
                strncpy(name, pe32.szExeFile, len);
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

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    SYSTEMTIME st;
    FILETIME ft;
    LARGE_INTEGER li;
    static char get_base_time_flag = 0;

    if (get_base_time_flag == 0) {
        get_base_time(&base_time);
    }

    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ft);

    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    li.QuadPart /= SECS_TO_FT_MULT;
    li.QuadPart -= base_time.QuadPart;

    tv->tv_sec = li.LowPart;
    tv->tv_usec = st.wMilliseconds*1000;

    return 0;
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

int get_nprocs()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}
