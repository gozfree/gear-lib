/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    netlink_kernel.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-16 01:13
 * updated: 2015-11-16 01:13
 ******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>


static int __init nlk_init(void)
{
	int rval = 0;
	return rval;
}

static void __exit nlk_exit(void)
{
}

module_init(nlk_init);
module_exit(nlk_exit);


MODULE_AUTHOR("gozfree <gozfree@163.com>");
MODULE_DESCRIPTION("IPC netlink driver");
MODULE_LICENSE("GPL");

