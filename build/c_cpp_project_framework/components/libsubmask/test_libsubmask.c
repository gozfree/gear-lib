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
#include "libsubmask.h"
#include <stdio.h>
#include <stdlib.h>

#if defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include <winsock.h>
#pragma comment(lib,"WS2_32.LIB")
#else
#include <arpa/inet.h>
#endif

int foo(char *ip,char *mask)
{
  unsigned int zipip; //ѹ����ip��ַ��������а�λ���㣩
  unsigned int zipmask; //ѹ������������
  int ipadd[4]; //ip��ַ����λ���ͱ�ʾ,�����м䲽��
  int maskadd[4]; //������������ͱ�ʾ
  int i = 0;
  struct in_addr ip1;
  unsigned int tmpip; 
	printf("foo ip:%s,mask:%s\n",ip,mask);
  if (submask_iserror(ip, mask) == 1)
  {
    return -1;
  }

  for (i = 0; i < 4; i++)
  {
    //���ַ��͵����������ip��ַת��Ϊ���ͱ�ʾ
    ipadd[i] = submask_sub(ip, i + 1);
    maskadd[i] = submask_sub(mask, i + 1);
  }

  /*�����������ip��ַѹ��*/
  zipmask = submask_zip(maskadd[0], maskadd[1], maskadd[2], maskadd[3]);
  zipip = submask_zip(ipadd[0], ipadd[1], ipadd[2], ipadd[3]);


  /*�ж������еĵڶ���������ֱ��������������뻹���������������λ��*/
  if (strlen(mask) > 3)
  {

    tmpip=submask_networkip(zipip, zipmask);
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("networkip:%s\n",inet_ntoa(ip1));
    tmpip=submask_firstip(zipip, zipmask);
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("firstip:%s\n",inet_ntoa(ip1));
    tmpip=submask_lastip(zipip, zipmask);
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("lastip:%s\n",inet_ntoa(ip1));
    tmpip=submask_broadcastip(zipip, zipmask);
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("broadcastip:%s\n",inet_ntoa(ip1));
  }
  else
  {
    int lmask = atoi(mask);
    tmpip=submask_networkip(zipip, submask_nmask(lmask));
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("networkip:%s\n",inet_ntoa(ip1));
    tmpip=submask_firstip(zipip, submask_nmask(lmask));
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("firstip:%s\n",inet_ntoa(ip1));
    tmpip=submask_lastip(zipip, submask_nmask(lmask));
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("lastip:%s\n",inet_ntoa(ip1));
    tmpip=submask_broadcastip(zipip, submask_nmask(lmask));
    memcpy(&ip1,&tmpip,sizeof(tmpip));
    printf("broadcastip:%s\n",inet_ntoa(ip1));
  }
	printf("------------------------------------------\n");
  return 0;
}
int foo1()
{
	char ipbuf[56];
	printf("foo1 submask_masktoprefix mask:%s ===> prefix:%d\n","255.255.255.128",submask_masktoprefix("255.255.255.128"));
	submask_prefixtomask(25,ipbuf);
	printf("foo1 submask_prefixtomask prefix:%d ===> mask:%s\n",25,ipbuf);
	return 0;
}

int main(int argc, char **argv)
{
		foo("192.168.1.1","255.255.255.0");
		foo("192.168.2.1","28");
		foo1();
    return 0;
}
