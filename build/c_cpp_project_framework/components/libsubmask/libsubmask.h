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
/*�����ַ*/
unsigned int submask_networkip(unsigned int ip, unsigned int mask);
/*���õĵ�һ��ip��ַ*/
unsigned int submask_firstip(unsigned int ip, unsigned int mask);
/*���õ����һ��ip��ַ*/
unsigned int submask_lastip(unsigned int ip, unsigned int mask);
/*�㲥��ַ*/
unsigned int submask_broadcastip(unsigned int ip, unsigned int mask);

/******************************************************************/
/*��������*/
/*�������������Ƿ����*/
int submask_iserror(char ip[16], char mask[16]);

/*��ȡ�ַ���ip��ַ��ָ������,��sub("202.99.160.68",1)����202*/
int submask_sub(char ip[16], int n);

/*���ĸ�int���ͱ�ʾ��ip��ַѹ����һ��int�Ա���а�λ����*/
unsigned int submask_zip(int a, int b, int c, int d);

/*��ѹ����ip��ַ���*/
unsigned int submask_unzip(unsigned int zipc);

/*ͨ�����������λ��������������*/
unsigned int submask_nmask(int n);

/*���볤��ǰ׺ת��Ϊ���������ַ*/
int submask_prefixtomask(int prefixlen, char* ip_str);

/*���������ַת��Ϊ���볤��ǰ׺*/
int submask_masktoprefix(const char* ip_str);
/******************************************************************/


#ifdef __cplusplus
}
#endif
#endif
