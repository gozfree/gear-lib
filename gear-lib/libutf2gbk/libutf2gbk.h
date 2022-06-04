#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef __LIBUTF2GBK_H__
#define __LIBUTF2GBK_H__
#ifdef __cplusplus
extern "C"
{
#endif

int  UTF_8ToGB2312(char*pOut, char *pInput, int pLen);
int UTF_8ToUnicode(char* pOutput, char *pInput);
void UnicodeToGB2312(char*pOut, char *pInput);

#ifdef __cplusplus
}
#endif
#endif /* __UTF2GBK_H__ */