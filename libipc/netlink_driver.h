/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
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
