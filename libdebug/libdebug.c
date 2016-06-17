/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdebug.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-17 16:53:13
 * updated: 2016-06-17 16:53:13
 *****************************************************************************/
#define __USE_GNU
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <ucontext.h>
#include <stdint.h>

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
  void *array[64];
  ucontext_t *uc    = (ucontext_t*)ucontext;
  void *caller_addr = (void*)uc->uc_mcontext.gregs[REG_EIP];
  size_t size       = 0;
  char **strings    = NULL;

  fprintf(stderr, "Caught signal %s(%d), address(%p) from %p!",
        strsignal(sig_num),
        sig_num,
        info->si_addr,
        caller_addr);

  size = backtrace(array, 64);
  array[1] = caller_addr; /* overwrite sigaction with caller's address */
  strings = backtrace_symbols(array, size);
  for (size_t i = 1; i < size && (NULL != strings); ++ i) {
    fprintf(stderr, "[bt]: (%zu) %s", i, strings[i]);
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
