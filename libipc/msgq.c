/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    msgq.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 17:24
 * updated: 2015-11-10 17:24
 *****************************************************************************/

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <libgzf.h>

struct mq_ctx {
    mqd_t mq;

};

static void *_mq_init()
{
  if (role == IPC_MSG_QUEUE_ROLE_SEND)
    oflag = O_RDWR | O_CREAT | O_EXCL ; //send is  block send
  else
    oflag = O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK; //receive is nonblock receive

    struct mq_ctx *mc = CALLOC(1, struct mq_ctx);
    mqd_t mq = mq_open(msg_queue_name, oflag , S_IRWXU | S_IRWXG, &attr);
    if (mq < 0) {
    }
}

static int _mq_send()
{
    mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio);

}
