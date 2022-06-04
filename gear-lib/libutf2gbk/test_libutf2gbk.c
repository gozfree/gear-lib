#include <stdio.h> 
#include <string.h>
#include "libutf2gbk.h"

int main()
{
	char buf[] = "nininihhahahhh你好呀你好呀哈哈哈";
	char buf2[(strlen(buf) + 1) * 6];
	
	UTF_8ToGB2312(buf2,buf,strlen(buf));
	printf("buf : %s \nbuf 2 : %s \n",buf,buf2);
	printf("utf8-len : %lu,gb2312-len : %lu\n",strlen(buf),strlen(buf2));
	return 0;
}