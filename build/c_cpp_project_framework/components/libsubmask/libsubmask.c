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
  //���������ip��ַ�Ƿ�Ψһ

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
  //����ip��ַ�����粿��
  unsigned int network = ip &mask;
#ifdef _DEBUG
  //printf("�����ַ:\t\t\t");
#endif
   return submask_unzip(network);
}

unsigned int submask_broadcastip(unsigned int ip, unsigned int mask)
{
  //���������ع㲥��ַ

  unsigned int network = ip &mask;
  unsigned int broadcast = ((network) | (~mask));
#ifdef _DEBUG
  //printf("�㲥��ַ:\t\t\t");
#endif
  return submask_unzip(broadcast);

}

unsigned int submask_firstip(unsigned int ip, unsigned int mask)
{
  //��������εĵ�һ������ip��ַ

  unsigned int network = ip &mask;
  unsigned int firstipadd = network + 1;
#ifdef _DEBUG
  //printf("���õĵ�һ��ip��ַ:\t\t");
#endif

  return submask_unzip(firstipadd);
}

unsigned int submask_lastip(unsigned int ip, unsigned int mask)
{
  //��������ε����һ������ip��ַ

  unsigned int network = ip &mask;
  unsigned int lastipadd = ((network) | (~mask)) - 1;
#ifdef _DEBUG
  //printf("���õ����һ��ip��ַ:\t\t");
#endif
 
  return submask_unzip(lastipadd);

}

unsigned int submask_zip(int a, int b, int c, int d)
{
  //���ĸ�int���ͱ�ʾ��ip��ַѹ����һ��int�Ա���а�λ����
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
  //��ѹ����ip��ַ���
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
  //��ȡ�ַ���ip��ַ��ָ������,��sub("202.99.160.68",1)����202
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
  //ͨ�����������λ��������������;

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

  re = re | 1; //�ڼ������������ʱ���Ƿ����ߵģ���Ϊ����ֻ��Ҫ�룱���л�����;
  for (i = 1; i < 32-n; i++)
  {
    re = re << 1;
    re = re | 1;
  }

  return ~re;
}

int _submask_testadd(char ip[16])
{
  //���Ե�ַ��ʽ

  int counp = 0; //��������ַ���м���'.'
  int i=0;

  for (i = 0; i < (int)strlen(ip); i++)
  //��ʼ����
  {
    if (ip[i] == '.')
    {
      counp++;
    }
  }

  if (counp != 3)
  //������ǣ����㷵��0
  {
    return 0;
  }
  //����ǣ�������ô�������ж�ÿ�����Ƿ���0��255֮��;

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
  //�����������λ���Ƿ�Ϸ�
  if (maskc <= 0 || maskc > 32)
  {
    return 0;
  }

  return 1;
}

int _submask_isnum(char add[16])
{
  int i=0;
  //���ip��ַ�����������Ƿ�ȫ������
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
  //�������������Ƿ����



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
        return 1; //�������벻����
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
      return 1; //��������λ������
    }
  }

  return 0;
}

int submask_iserror(char ip[16], char mask[16])
{
  //�ж��û�����Ĳ����Ƿ��������������ʾ�����أ�

  if (_submask_isnum(ip) == 1 || _submask_isnum(mask) == 1)
  {
    printf("�����д��ڷǷ��ַ�!\n");
    return 1;
  }

  if (strlen(mask) > 2)
  {

    if (_submask_testadd(ip) == 0)
    {
      printf("ip��ַ��ʽ����!\n");
      return 1;
    }
    else if (_submask_testadd(mask) == 0)
    {
      printf("���������ʽ����!\n");
      return 1;
    }
    else if (_submask_unmask(ip, mask) == 1)
    {
      printf("�������벻����!\n");
      return 1;
    }
    else if (_submask_isone(mask) == 1)
    {
      printf("Ψһ��ip��ַ:%s\n", ip);
      return 1;
    }
  }
  else
  {

    if (_submask_testadd(ip) == 0)
    {
      printf("ip��ַ��ʽ����!\n");
      return 1;
    }
    else if (_submask_testmaskc(atoi(mask)) == 0)
    {
      printf("���������С����!\n");
      return 1;
    }
    else if (_submask_unmask(ip, mask) == 1)
    {
      printf("��������λ������!\n");
      return 1;
    }
    else if ((atoi(mask) == 32) || (atoi(mask) == 0))
    {
      printf("Ψһ��ip��ַ:%s\n", ip);
      return 1;
    }
  }

  return 0;
}

