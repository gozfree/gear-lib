/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#ifndef LIBSUBMASK_H
#define LIBSUBMASK_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
	#pragma warning (disable:4996)
#endif
/*网络地址*/
unsigned int submask_networkip(unsigned int ip, unsigned int mask);
/*可用的第一个ip地址*/
unsigned int submask_firstip(unsigned int ip, unsigned int mask);
/*可用的最后一个ip地址*/
unsigned int submask_lastip(unsigned int ip, unsigned int mask);
/*广播地址*/
unsigned int submask_broadcastip(unsigned int ip, unsigned int mask);

/******************************************************************/
/*辅助函数*/
/*测试子网掩码是否合理*/
int submask_iserror(char ip[16], char mask[16]);

/*提取字符型ip地址中指定部分,如sub("202.99.160.68",1)返回202*/
int submask_sub(char ip[16], int n);

/*将四个int类型表示的ip地址压缩进一个int以便进行按位计算*/
unsigned int submask_zip(int a, int b, int c, int d);

/*将压缩的ip地址输出*/
unsigned int submask_unzip(unsigned int zipc);

/*通过子网掩码的位数计算子网掩码*/
unsigned int submask_nmask(int n);

/*掩码长度前缀转换为子网掩码地址*/
int submask_prefixtomask(int prefixlen, char* ip_str);

/*子网掩码地址转换为掩码长度前缀*/
int submask_masktoprefix(const char* ip_str);
/******************************************************************/


#ifdef __cplusplus
}
#endif
#endif
