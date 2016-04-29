/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    shm.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 18:36
 * updated: 2015-11-10 18:36
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "libipc.h"


struct shm_ctx {

};


static void *shm_init(const char *name, enum ipc_role role)
{
    return NULL;
}


static void shm_deinit(struct ipc *ipc)
{

}


static int shm_write(struct ipc *ipc, const void *buf, size_t len)
{
    return 0;
}

static int shm_read(struct ipc *ipc, void *buf, size_t len)
{
    return 0;
}


struct ipc_ops shm_ops = {
    .init             = shm_init,
    .deinit           = shm_deinit,
    .accept           = NULL,
    .connect          = NULL,
    .register_recv_cb = NULL,
    .send             = shm_write,
    .recv             = shm_read,
};
