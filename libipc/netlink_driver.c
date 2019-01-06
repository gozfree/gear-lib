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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/netlink.h>
#include <linux/spinlock.h>
#include <net/sock.h>
#include "netlink_driver.h"

#define IPC_SERVER_PREFIX   "/IPC_SERVER."
#define IPC_CLIENT_PREFIX   "/IPC_CLIENT."


static struct sock *nlfd;

#define print_buffer(buf, len)                                  \
    do {                                                        \
        int _i;                                                 \
        if (!buf || len <= 0) {                                 \
            printk("%s:%d invalid buf=%p, len=%d\n", __func__, __LINE__, buf, len);       \
            break;                                              \
        }                                                       \
        for (_i = 0; _i < len; _i++) {                          \
            if (!(_i%16))                                       \
                printk("\n%p: ", buf+_i);                       \
            printk("%02x ", (*((char *)buf + _i)) & 0xff);      \
        }                                                       \
	printk("\n");                                           \
    } while (0)

static int nl_send_msg(int pid, const u8 *data, int data_len, int dir)
{
	int ret;
	struct nlmsghdr *rep;
	u8 *res;
	struct sk_buff *skb;

	skb = nlmsg_new(data_len, GFP_KERNEL);
	if(!skb) {
		printk("nlmsg_new failed!!!\n");
		return -ENOMEM;
	}

	rep = __nlmsg_put(skb, pid, 0, dir, data_len, 0);
	res = nlmsg_data(rep);
	memcpy(res, data, data_len);
	//print_buffer(rep, rep->nlmsg_len);
	ret = netlink_unicast(nlfd, skb, pid, 0);
	printk("%s:%d ret = %d, pid = %d, data_len = %d\n", __func__, __LINE__,
			ret, pid, data_len);
	//nlmsg_free(skb);
	return 0;
}

#if 0
static int nl_broadcast_msg(int pid, int group, const u8 *data, int data_len)
{
	int ret;
	struct nlmsghdr *rep;
	u8 *res;
	struct sk_buff *skb;

	skb = nlmsg_new(data_len, GFP_KERNEL);
	if(!skb) {
		printk("nlmsg_new failed!!!\n");
		return -ENOMEM;
	}

	rep = __nlmsg_put(skb, pid, 0, NLMSG_NOOP, data_len, 0);
	res = nlmsg_data(rep);
	memcpy(res, data, data_len);
	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = group;
	//ret = netlink_broadcast(nlfd, skb, 0, NETLINK_IPC_GROUP_CLIENT, GFP_KERNEL);
	ret = nlmsg_multicast(nlfd, skb, 0, NETLINK_IPC_GROUP_CLIENT, GFP_KERNEL);
	printk("%s:%d ret = %d, pid = %d, data_len = %d\n", __func__, __LINE__,
			ret, pid, data_len);
	//nlmsg_free(skb);
	return 0;
}
#endif

static int parse_msg(int type, char *msg, int len)
{
    int dir;
    printk("%s:%d msg = %s\n", __func__, __LINE__, msg);
    if (!strncmp(msg, IPC_SERVER_PREFIX, strlen(IPC_SERVER_PREFIX))) {
        dir = SERVER_TO_SERVER;
    } else if (!strncmp(msg, IPC_CLIENT_PREFIX, strlen(IPC_CLIENT_PREFIX))) {
        dir = CLIENT_TO_CLIENT;
    } else {
        dir = type;
        printk("%s:%d nlmsg_type %d\n", __func__, __LINE__, dir);
    }
    return dir;
}

static void nl_recv_msg_handler(struct sk_buff * skb)
{
	struct nlmsghdr * nlhdr = NULL;
	int len;
	int msg_len;
	char *msg;
	int towards;

	nlhdr = nlmsg_hdr(skb);
	len = skb->len;

	for(; NLMSG_OK(nlhdr, len); nlhdr = NLMSG_NEXT(nlhdr, len)) {
		if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
			printk("NETLINK ERR: Corruptted msg!\n");
			continue;
		}
		msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
		printk("%s:%d skb->len=%d,nlmsg_len=%d,msg_len=%d, buf=%s\n", __func__, __LINE__, skb->len, nlhdr->nlmsg_len, msg_len, (char *)NLMSG_DATA(nlhdr));
		msg = NLMSG_DATA(nlhdr);
		//print_buffer(nlhdr, nlhdr->nlmsg_len);
		towards = parse_msg(nlhdr->nlmsg_type, msg, msg_len);
		switch (towards) {
			case SERVER_TO_SERVER:
				printk("%s:%d SERVER_TO_SERVER\n", __func__, __LINE__);
				nl_send_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), msg_len, SERVER_TO_SERVER);
				break;
			case CLIENT_TO_CLIENT:
				printk("%s:%d CLIENT_TO_CLIENT\n", __func__, __LINE__);
				nl_send_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), msg_len, CLIENT_TO_CLIENT);
				break;
			case SERVER_TO_CLIENT:
				printk("%s:%d SERVER_TO_CLIENT\n", __func__, __LINE__);
				//nl_broadcast_msg(NETLINK_CB(skb).portid, NETLINK_IPC_GROUP_CLIENT, NLMSG_DATA(nlhdr), msg_len);
				nl_send_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), msg_len, SERVER_TO_CLIENT);
				break;
			case CLIENT_TO_SERVER:
				printk("%s:%d CLIENT_TO_SERVER\n", __func__, __LINE__);
				//nl_broadcast_msg(NETLINK_CB(skb).portid, NETLINK_IPC_GROUP_SERVER, NLMSG_DATA(nlhdr), msg_len);
				nl_send_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), msg_len, CLIENT_TO_SERVER);
				break;
			default:
				printk("%s:%d to unknown\n", __func__, __LINE__);
				//nl_broadcast_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), msg_len);
				break;
		}
		//nl_send_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), msg_len);
	}
}

static int __init nlk_init(void)
{
	struct netlink_kernel_cfg cfg;

	cfg.groups = 0;
	cfg.input = nl_recv_msg_handler;

	nlfd = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if (!nlfd) {
		printk("netlink_kernel_create failed\n");
		return -1;
	}
	printk("netlink_driver init success.\n");

	return 0;
}

static void __exit nlk_exit(void)
{
	if (nlfd) {
		netlink_kernel_release(nlfd);
	}
	printk("netlink_driver exit success.\n");
}

module_init(nlk_init);
module_exit(nlk_exit);

MODULE_AUTHOR("gozfree <gozfree@163.com>");
MODULE_DESCRIPTION("IPC (Inter-process communication) netlink driver");
MODULE_LICENSE("GPL");
