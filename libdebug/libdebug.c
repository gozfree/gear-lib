/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdebug.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-17 16:53:13
 * updated: 2016-06-17 16:53:13
 *****************************************************************************/
#ifndef __cplusplus
#define _GNU_SOURCE
#endif
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <execinfo.h>

#include "libdebug.h"

#define BT_SIZE 100
#define CMD_SIZE 1024

/*
 * from /usr/include/bits/signum.h
 */
static uint32_t signum[] =
{
    SIGILL,  /* Illegal instruction (ANSI).  */
    SIGABRT, /* Abort (ANSI).  */
    SIGBUS,  /* BUS error (4.2 BSD). (unaligned access) */
    SIGFPE,  /* Floating-point exception (ANSI).  */
    SIGSEGV, /* Segmentation violation (ANSI).  */
};

static void *get_uc_mcontext_pc(ucontext_t *uc)
{
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
    /* OSX < 10.6 */
    #if defined(__x86_64__)
    return (void*) uc->uc_mcontext->__ss.__rip;
    #elif defined(__i386__)
    return (void*) uc->uc_mcontext->__ss.__eip;
    #else
    return (void*) uc->uc_mcontext->__ss.__srr0;
    #endif
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
    /* OSX >= 10.6 */
    #if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
    return (void*) uc->uc_mcontext->__ss.__rip;
    #else
    return (void*) uc->uc_mcontext->__ss.__eip;
    #endif
#elif defined(__linux__)
    /* Linux */
    #if defined(__i386__)
    return (void*) uc->uc_mcontext.gregs[REG_EIP]; /* Linux 32 */
    #elif defined(__X86_64__) || defined(__x86_64__)
    return (void*) uc->uc_mcontext.gregs[REG_RIP]; /* Linux 64 */
    #elif defined(__ia64__) /* Linux IA64 */
    return (void*) uc->uc_mcontext.sc_ip;
    #elif defined(__arm__)
    return (void*) uc->uc_mcontext.arm_pc;
    #endif
#else
    return NULL;
#endif
}

static void backtrace_symbols_detail(void *array[], int size)
{
    int i;
    char cmd[CMD_SIZE] = "addr2line -C -f -e ";
    char* prog = cmd + strlen(cmd);
    int r = readlink("/proc/self/exe", prog, sizeof(cmd) - (prog-cmd)-1);
    if (r == -1) {
        fprintf(stderr, "backtrace_symbols_detail unsupported!\n");
        perror("readlink");
        return;
    }
    prog[r] = '\0';
    FILE* fp = popen(cmd, "w");
    if (!fp) {
        perror("popen");
        return;
    }
    for (i = 1; i < size; ++i) {//from 1, ignore this file info
        fprintf(fp, "%p\n", array[i]);
    }
    fclose(fp);
}

static void backtrace_handler(int sig_num, siginfo_t *info, void *ucontext)
{
    void *array[BT_SIZE];
    int i, size = 0;
    char **strings = NULL;
    void *caller_addr = get_uc_mcontext_pc((ucontext_t *)ucontext);
    fprintf(stderr, "Program received signal %s.\n", strsignal(sig_num));
    fprintf(stderr, "%d:%s [%p]\n", sig_num, strerror(sig_num), caller_addr);

    size = backtrace(array, BT_SIZE);
    /* overwrite sigaction with caller's address, but seems no needed */
    array[1] = caller_addr;
    strings = backtrace_symbols(array, size);
    for (i = 0; i < size; ++ i) {
        fprintf(stderr, "#%d  %s\n", i, strings[i]);
    }
    backtrace_symbols_detail(array, size);

    exit(EXIT_FAILURE);
}

int debug_backtrace_init()
{
    struct sigaction sigact;
    sigact.sa_sigaction = backtrace_handler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;
    int ret = 0;
    for (uint32_t i = 0; i < (sizeof(signum) / sizeof(uint32_t)); ++i) {
        if (sigaction(signum[i], &sigact, NULL) != 0) {
            fprintf(stderr, "Failed to set signal handler for %s(%d)!",
                  strsignal(signum[i]),
                  signum[i]);
            ret = -1;
            break;
        }
    }
    return ret;
}

void debug_backtrace_dump()
{
    void* buffer[BT_SIZE];
    int size = backtrace(buffer, BT_SIZE);
    backtrace_symbols_detail(buffer, size);
}
