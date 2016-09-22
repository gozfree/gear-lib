/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    netlink_driver.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#ifndef NETLINK_DRIVER_H
#define NETLINK_DRIVER_H


#define NETLINK_IPC_GROUP	0	//must be zero
#define NETLINK_IPC_GROUP_CLIENT 1
#define NETLINK_IPC_GROUP_SERVER 2

struct nl_msg_wrap {
    int dir;
    void *msg;
};

typedef enum msg_towards {
    SERVER_TO_SERVER = 0,
    SERVER_TO_CLIENT = 1,
    CLIENT_TO_SERVER = 2,
    CLIENT_TO_CLIENT = 3,

    UNKNOWN_TO_UNKNOWN = 4,
} msg_towards;



#endif
