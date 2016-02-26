/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    netlink.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 18:36
 * updated: 2015-11-10 18:36
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


#define IMP2_U_PID   0
#define IMP2_K_MSG   1
#define IMP2_CLOSE   2

#define NL_IMP2      31

struct packet_info
{
  __u32 src;
  __u32 dest;
};

static struct sock *nlfd;

struct
{
	__u32 pid;
	rwlock_t lock;
}user_proc;

struct iav_nl_request
{
	u32 request_id;
	u32 responded;
	wait_queue_head_t wq_request;
};

struct iav_nl_obj
{
	s32 nl_user_pid;
	u32 nl_connected;
	s32 nl_init;
	s32 nl_port;
	s32 nl_session_count;
	s32 nl_request_count;
	struct sock *nl_sock;
};

enum NL_MSG_TYPE {
	NL_MSG_TYPE_SESSION = 0,
	NL_MSG_TYPE_REQUEST,
};

enum NL_MSG_DIR {
	NL_MSG_DIR_CMD = 0,
	NL_MSG_DIR_STATUS,
};

#if 0
DEFINE_SEMAPHORE(receive_sem);
static void kernel_receive(struct sock *sk, int len)
{
	do
	{
		struct sk_buff *skb;
		if(down_trylock(&receive_sem))
			return;

		while((skb = skb_dequeue(&sk->receive_queue)) != NULL)
		{
			{
				struct nlmsghdr *nlh = NULL;

				if(skb->len >= sizeof(struct nlmsghdr))
				{
					nlh = (struct nlmsghdr *)skb->data;
					if((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
							&& (skb->len >= nlh->nlmsg_len))
					{
						if(nlh->nlmsg_type == IMP2_U_PID)
						{
							write_lock_bh(&user_proc.pid);
							user_proc.pid = nlh->nlmsg_pid;
							write_unlock_bh(&user_proc.pid);
						}
						else if(nlh->nlmsg_type == IMP2_CLOSE)
						{
							write_lock_bh(&user_proc.pid);
							if(nlh->nlmsg_pid == user_proc.pid)
								user_proc.pid = 0;
							write_unlock_bh(&user_proc.pid);
						}
					}
				}
			}
			kfree_skb(skb);
		}
		up(&receive_sem);
	}while(nlfd && nlfd->receive_queue.qlen);
}

static int send_to_user(struct packet_info *info)
{
	int ret;
	int size;
	unsigned char *old_tail;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	struct packet_info *packet;

	size = NLMSG_SPACE(sizeof(*info));

	skb = alloc_skb(size, GFP_ATOMIC);
	old_tail = skb->tail;

	nlh = NLMSG_PUT(skb, 0, 0, IMP2_K_MSG, size-sizeof(*nlh));
	packet = NLMSG_DATA(nlh);
	memset(packet, 0, sizeof(struct packet_info));

	packet->src = info->src;
	packet->dest = info->dest;

	nlh->nlmsg_len = skb->tail - old_tail;
	NETLINK_CB(skb).dst_groups = 0;

	read_lock_bh(&user_proc.lock);
	ret = netlink_unicast(nlfd, skb, user_proc.pid, MSG_DONTWAIT);
	read_unlock_bh(&user_proc.lock);

	return ret;

nlmsg_failure:
	if(skb)
		kfree_skb(skb);
	return -1;
}

static unsigned int get_icmp(unsigned int hook,
		struct sk_buff **pskb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct packet_info info;

	if(iph->protocol == IPPROTO_ICMP)
	{
		read_lock_bh(&user_proc.lock);
		if(user_proc.pid != 0)
		{
			read_unlock_bh(&user_proc.lock);
			info.src = iph->saddr;
			info.dest = iph->daddr;
			send_to_user(&info);
		}
		else
			read_unlock_bh(&user_proc.lock);
	}

	return NF_ACCEPT;
}

static struct nf_hook_ops imp2_ops =
{
	.hook = get_icmp,
	.pf = PF_INET,
	.hooknum = NF_IP_PRE_ROUTING,
	.priority = NF_IP_PRI_FILTER -1,
};
#endif

#if 0
static int nlk_probe(struct platform_device *pdev)
{
	printk("%s:%s:%d xxxx\n", __FILE__, __func__, __LINE__);
	return 0;
}

static int nlk_remove(struct platform_device *pdev)
{
	printk("%s:%s:%d xxxx\n", __FILE__, __func__, __LINE__);
	return 0;
}

static struct platform_driver nlk_driver = {
	.probe = nlk_probe,
	.remove = nlk_remove,
	.driver = {
		.name = "netlink",
		.owner = THIS_MODULE,
	},
};
#endif

#define NETLINK_IPC_GROUP	0
#define SCSI_NL_SHOST_VENDOR			0x0001

struct scsi_nl_hdr {
	uint8_t version;
	uint8_t transport;
	uint16_t magic;
	uint16_t msgtype;
	uint16_t msglen;
} __attribute__((aligned(sizeof(uint64_t))));

#define print_buffer(buf, len)                                  \
    do {                                                        \
        int _i;                                                 \
        if (!buf || len <= 0) {                                 \
            break;                                              \
        }                                                       \
        for (_i = 0; _i < len; _i++) {                          \
            if (!(_i%16))                                       \
                printk("\n%p: ", buf+_i);                       \
            printk("%02x ", (*((char *)buf + _i)) & 0xff);      \
        }                                                       \
    } while (0)


static int nl_send_msg(int pid, const u8 *data, int data_len)
{
    struct nlmsghdr *rep;
    u8 *res;
    struct sk_buff *skb;

    skb = nlmsg_new(data_len, GFP_KERNEL);
    if(!skb) {
       printk("nlmsg_new failed!!!\n");
       return -1;
    }

    rep = __nlmsg_put(skb, pid, 0, NLMSG_NOOP, data_len, 0);
    res = nlmsg_data(rep);
    memcpy(res, data, data_len);
    netlink_unicast(nlfd, skb, pid, 0);
    printk("%s:%s:%d pid = %d, data_len = %d\n", __FILE__, __func__, __LINE__, pid, data_len);
    return 0;
}

static void nl_recv_msg_handler(struct sk_buff * skb)
{
	struct nlmsghdr * nlhdr = NULL;
	int len;
	int msg_len;

	nlhdr = nlmsg_hdr(skb);
	len = skb->len;

	for(; NLMSG_OK(nlhdr, len); nlhdr = NLMSG_NEXT(nlhdr, len)) {
		if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
			printk("NETLINK ERR: Corruptted msg!\n");
			continue;
		}
		msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
		printk("%s:%s:%d nlhdr->nlmsg_pid=%d, nlmsg_len=%d\n", __FILE__, __func__, __LINE__, nlhdr->nlmsg_pid, nlhdr->nlmsg_len);
		//print_buffer(NLMSG_DATA(nlhdr), nlhdr->nlmsg_len);
		nl_send_msg(NETLINK_CB(skb).portid, NLMSG_DATA(nlhdr), nlhdr->nlmsg_len);
	}
}

static void nlk_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	struct scsi_nl_hdr *hdr;
	u32 rlen;
	int err, tport;

	while (skb->len >= NLMSG_HDRLEN) {
		err = 0;

		nlh = nlmsg_hdr(skb);
		if ((nlh->nlmsg_len < (sizeof(*nlh) + sizeof(*hdr))) ||
		    (skb->len < nlh->nlmsg_len)) {
			printk(KERN_WARNING "%s: discarding partial skb\n",
				 __func__);
			return;
		}

		rlen = NLMSG_ALIGN(nlh->nlmsg_len);
		if (rlen > skb->len)
			rlen = skb->len;

#if 0
		if (nlh->nlmsg_type != SCSI_TRANSPORT_MSG) {
			err = -EBADMSG;
			goto next_msg;
		}
#endif

		hdr = nlmsg_data(nlh);
#if 0
		if ((hdr->version != SCSI_NL_VERSION) ||
		    (hdr->magic != SCSI_NL_MAGIC)) {
			err = -EPROTOTYPE;
			goto next_msg;
		}
#endif

		if (!netlink_capable(skb, CAP_SYS_ADMIN)) {
			err = -EPERM;
			goto next_msg;
		}

		if (nlh->nlmsg_len < (sizeof(*nlh) + hdr->msglen)) {
			printk(KERN_WARNING "%s: discarding partial message\n",
				 __func__);
			goto next_msg;
		}

		/*
		 * Deliver message to the appropriate transport
		 */
		tport = hdr->transport;
		if (tport == NETLINK_IPC_PORT) {
			switch (hdr->msgtype) {
			case SCSI_NL_SHOST_VENDOR:
				/* Locate the driver that corresponds to the message */
				err = -ESRCH;
				break;
			default:
				err = -EBADR;
				break;
			}
			if (err)
				printk(KERN_WARNING "%s: Msgtype %d failed - err %d\n",
				       __func__, hdr->msgtype, err);
		}
		else
			err = -ENOENT;

next_msg:
		if ((err) || (nlh->nlmsg_flags & NLM_F_ACK))
			netlink_ack(skb, nlh, err);

		skb_pull(skb, rlen);
	}
}

static int __init nlk_init(void)
{
	struct netlink_kernel_cfg cfg;

	cfg.groups = NETLINK_IPC_GROUP;
	cfg.input = nlk_recv_msg;
	cfg.input = nl_recv_msg_handler;

	nlfd = netlink_kernel_create(&init_net, NETLINK_IPC_PORT, &cfg);
	if (!nlfd) {
		printk("can not create a netlink socket\n");
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

