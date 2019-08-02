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



int _submask_isone(char mask[16]);
int _submask_isnum(char add[16]);
int _submask_testmaskc(int maskc);
int _submask_testadd(char ip[16]);

int _submask_unmask(char ip[16], char mask[16]);

int submask_masktoprefix(const char* ip_str)
{
    //int ret = 0;
    unsigned int ip_num = 0;
    unsigned int c1,c2,c3,c4;
    int cnt = 0;
		int i = 0;
    sscanf(ip_str, "%u.%u.%u.%u", &c1, &c2, &c3, &c4);
    ip_num = c1<<24 | c2<<16 | c3<<8 | c4;

    // fast...
    if (ip_num == 0xffffffff) return 32;
    if (ip_num == 0xffffff00) return 24;
    if (ip_num == 0xffff0000) return 16;
    if (ip_num == 0xff000000) return 6;

    // just in case
    for (i = 0; i < 32; i++)
    {
        //unsigned int tmp = (ip_num<<i);
        //printf("%d tmp: %x\n", i+1, tmp);
        if ((ip_num<<i) & 0x80000000)
            cnt++;
        else
            break;
    }
    
    //printf("cnt: %d\n", cnt);
    return cnt;
}


int submask_prefixtomask(int prefixlen, char* ip_str)
{
    //char tmp[16] = {0};
    //char* p = tmp;
    unsigned int ip_num = 0;
    int i=0;
    int j=0;


    if (ip_str == NULL) return -1;
    if (prefixlen > 32) return -1;


    // fast...
    if (prefixlen == 8) strcpy(ip_str, "255.0.0.0");
    if (prefixlen == 16) strcpy(ip_str, "255.255.0.0");
    if (prefixlen == 24) strcpy(ip_str, "255.255.255.0");
    if (prefixlen == 32) strcpy(ip_str, "255.255.255.255");


    // just in case
    for (i = prefixlen, j = 31; i > 0; i--, j--)
    {
        //unsigned int tmp = (1<<j);
        //printf("%d tmp: %08x\n", i, tmp);
        ip_num += (1<<j);
    }
    //printf("ip_num: %08x\n", ip_num);
    sprintf(ip_str, "%hhu.%hhu.%hhu.%hhu", (ip_num>>24)&0xff, (ip_num>>16)&0xff, (ip_num>>8)&0xff, ip_num&0xff);


    return 0;
}


int _submask_isone(char mask[16])
{
  //检测网段内ip地址是否唯一

  if ((submask_sub(mask, 1) == 255 && submask_sub(mask, 2) == 255 && submask_sub(mask, 3) == 255 && submask_sub
    (mask, 4) == 255) || (submask_sub(mask, 1) == 0 && submask_sub(mask, 2) == 0 && submask_sub(mask, 3)
    == 0 && submask_sub(mask, 4) == 0))
  {
    return 1;
  }
  return 0;
}

unsigned int submask_networkip(unsigned int ip, unsigned int mask)
{
  //计算ip地址的网络部分
  unsigned int network = ip &mask;
#ifdef _DEBUG
  //printf("网络地址:\t\t\t");
#endif
   return submask_unzip(network);
}

unsigned int submask_broadcastip(unsigned int ip, unsigned int mask)
{
  //计算该网络地广播地址

  unsigned int network = ip &mask;
  unsigned int broadcast = ((network) | (~mask));
#ifdef _DEBUG
  //printf("广播地址:\t\t\t");
#endif
  return submask_unzip(broadcast);

}

unsigned int submask_firstip(unsigned int ip, unsigned int mask)
{
  //计算该网段的第一个可用ip地址

  unsigned int network = ip &mask;
  unsigned int firstipadd = network + 1;
#ifdef _DEBUG
  //printf("可用的第一个ip地址:\t\t");
#endif

  return submask_unzip(firstipadd);
}

unsigned int submask_lastip(unsigned int ip, unsigned int mask)
{
  //计算该网段的最后一个可用ip地址

  unsigned int network = ip &mask;
  unsigned int lastipadd = ((network) | (~mask)) - 1;
#ifdef _DEBUG
  //printf("可用的最后一个ip地址:\t\t");
#endif
 
  return submask_unzip(lastipadd);

}

unsigned int submask_zip(int a, int b, int c, int d)
{
  //将四个int类型表示的ip地址压缩进一个int以便进行按位计算
  unsigned int re = 0;

  re = re | (unsigned int)a;
  re = re << 8;

  re = re | (unsigned int)b;
  re = re << 8;

  re = re | (unsigned int)c;
  re = re << 8;

  re = re | (unsigned int)d;

  return re;
}

unsigned int submask_unzip(unsigned int zipc)
{
  //将压缩的ip地址输出
  unsigned int stmpip;
  unsigned char ctmpip[4];
  ctmpip[0] = zipc >> 24;
  ctmpip[1] = (zipc << 8) >> 24;
  ctmpip[2] = (zipc << 8 << 8) >> 24;
  ctmpip[3] = zipc &255;
  memcpy(&stmpip,ctmpip,sizeof(unsigned int));
#ifdef _DEBUG
 /* printf("%d.", zipc >> 24);
  printf("%d.", (zipc << 8) >> 24);
  printf("%d.", (zipc << 8 << 8) >> 24);
  printf("%d\n", zipc &255);*/
#endif
  return stmpip;

}

int submask_sub(char ip[16], int n)
{
  //提取字符型ip地址中指定部分,如sub("202.99.160.68",1)返回202
  int daoat[3];
  char ippart[4] = "";
  int i=0;
  int j=0;

  for (i = 0, j = 0; i < 16; i++)
  {
    if (ip[i] == '.')
    {
      daoat[j] = i;
      j++;
    }
    if (j == 3)
    {
      break;
    }

  }

  
  if (n == 1)
  {
    return atoi(ip);
  }
  else if (n == 2)
  {
    for (j = 0, i = 1+daoat[0]; i < daoat[1]; i++, j++)
    {
      ippart[j] = ip[i];
    }
  }
  else if (n == 3)
  {
    for (j = 0, i = 1+daoat[1]; i < daoat[2]; i++, j++)
    {
      ippart[j] = ip[i];
    }
  }
  else
  {
    for (j = 0, i = 1+daoat[2]; i < (int)strlen(ip); i++, j++)
    {
      ippart[j] = ip[i];
    }
  }

  return atoi(ippart);
}

unsigned int submask_nmask(int n)
{
  //通过子网掩码的位数计算子网掩码;

  unsigned int re = 0;
  int i=0;

  if (n == 0)
  {
    return re;
  }
  if (n == 32)
  {
    return ~re;
  }

  re = re | 1; //在计算子网掩码的时候是反着走的，因为这样只需要与１进行或运算;
  for (i = 1; i < 32-n; i++)
  {
    re = re << 1;
    re = re | 1;
  }

  return ~re;
}

int _submask_testadd(char ip[16])
{
  //测试地址格式

  int counp = 0; //用于数地址中有几个'.'
  int i=0;

  for (i = 0; i < (int)strlen(ip); i++)
  //开始数点
  {
    if (ip[i] == '.')
    {
      counp++;
    }
  }

  if (counp != 3)
  //如果不是３个点返回0
  {
    return 0;
  }
  //如果是３个点那么接下来判断每部分是否都在0和255之间;

  else if (submask_sub(ip, 1) < 0 || submask_sub(ip, 1) > 255)
  {
    return 0;
  }
  else if (submask_sub(ip, 2) < 0 || submask_sub(ip, 2) > 255)
  {
    return 0;
  }
  else if (submask_sub(ip, 3) < 0 || submask_sub(ip, 3) > 255)
  {
    return 0;
  }

  else if (submask_sub(ip, 4) < 0 || submask_sub(ip, 4) > 255)
  {
    return 0;
  }

  return 1;
}

int _submask_testmaskc(int maskc)
{
  //检测子网掩码位过是否合法
  if (maskc <= 0 || maskc > 32)
  {
    return 0;
  }

  return 1;
}

int _submask_isnum(char add[16])
{
  int i=0;
  //检查ip地址或子网掩码是否全是数字
  for (i = 0; i < (int)strlen(add); i++)
  {
    if (strchr(" 1234567890.", add[i])==NULL)
    {
      return 1;
    }
  }
  return 0;
}

int _submask_unmask(char ip[16], char mask[16])
{
  //测试子网掩码是否合理



  if (strlen(mask) > 2)
  {
  	int c;
    unsigned int i = submask_zip(submask_sub(mask, 1), submask_sub(mask, 2), submask_sub(mask, 3), submask_sub(mask, 4))
      ;
    i = ~i;
    
    for (c = 1; c <= 32; c++)
    {
      if ((i &1) == 0)
        break;
      i = i >> 1;
    }
    
    for (c++; c <= 32; c++)
    {
      if ((i &1) == 1)
      {
        return 1; //子网掩码不连续
      }
      i = i >> 1;
    }

  }
  else
  {
    unsigned int i = submask_zip(submask_sub(ip, 1), submask_sub(ip, 2), submask_sub(ip, 3), submask_sub(ip, 4));
    i = i >> (32-atoi(mask));
    if (i == 0)
    {
      return 1; //子网掩码位数过少
    }
  }

  return 0;
}

int submask_iserror(char ip[16], char mask[16])
{
  //判断用户输入的参数是否有误，有则给出提示并返回１

  if (_submask_isnum(ip) == 1 || _submask_isnum(mask) == 1)
  {
    printf("参数中存在非法字符!\n");
    return 1;
  }

  if (strlen(mask) > 2)
  {

    if (_submask_testadd(ip) == 0)
    {
      printf("ip地址格式错误!\n");
      return 1;
    }
    else if (_submask_testadd(mask) == 0)
    {
      printf("子网掩码格式错误!\n");
      return 1;
    }
    else if (_submask_unmask(ip, mask) == 1)
    {
      printf("子网掩码不连续!\n");
      return 1;
    }
    else if (_submask_isone(mask) == 1)
    {
      printf("唯一的ip地址:%s\n", ip);
      return 1;
    }
  }
  else
  {

    if (_submask_testadd(ip) == 0)
    {
      printf("ip地址格式错误!\n");
      return 1;
    }
    else if (_submask_testmaskc(atoi(mask)) == 0)
    {
      printf("子网掩码大小错误!\n");
      return 1;
    }
    else if (_submask_unmask(ip, mask) == 1)
    {
      printf("子网掩码位数过少!\n");
      return 1;
    }
    else if ((atoi(mask) == 32) || (atoi(mask) == 0))
    {
      printf("唯一的ip地址:%s\n", ip);
      return 1;
    }
  }

  return 0;
}

