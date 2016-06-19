/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdebug.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-17 16:53:13
 * updated: 2016-06-17 16:53:13
 *****************************************************************************/
#define _GNU_SOURCE
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <execinfo.h>

#include "libdebug.h"

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

static void critical_error_handler(int sig_num, siginfo_t *info, void *ucontext)
{
#define BT_SIZE 100
    void *array[BT_SIZE];
    ucontext_t *uc    = (ucontext_t*)ucontext;
    void *caller_addr = NULL;
#ifdef __x86_64__
    caller_addr = (void*)uc->uc_mcontext.gregs[REG_RIP];
#else
    caller_addr = (void*)uc->uc_mcontext.gregs[REG_EIP];
#endif
    size_t size       = 0;
    char **strings    = NULL;

    fprintf(stderr, "Program received signal %s.\n", strsignal(sig_num));
    fprintf(stderr, "%p %s:%d, address(%p)\n",
                    caller_addr,
                    strerror(sig_num),
                    sig_num,
                    info->si_addr);

    size = backtrace(array, BT_SIZE);

    //array[0] = caller_addr; /* overwrite sigaction with caller's address */
    strings = backtrace_symbols(array, size);
    for (size_t i = 0; i < size && (NULL != strings); ++ i) {
        fprintf(stderr, "#%zu %s\n", i, strings[i]);
    }
    exit(EXIT_FAILURE);
}

int debug_register_backtrace()
{
  struct sigaction sigact;
  sigact.sa_sigaction = critical_error_handler;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;

  int ret = 0;

  for (uint32_t i = 0; i < (sizeof(signum) / sizeof(uint32_t)); ++ i) {
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
