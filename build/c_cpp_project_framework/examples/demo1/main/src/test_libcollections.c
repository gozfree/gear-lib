/******************************************************************************
 * Copyright (C) 2014-2020 dianjixz <18637716021@163.com>
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
#include "libcollections.h"
#include <stdio.h>
#include <stdlib.h>
#include "test.h"

void test_lifo()
{
    printf("------------------test lifo------------------\n");
    lifo_t tmp;
    char tmp_c;
    lifo_alloc(&tmp, 5, 1);
    printf("this lifo size:%d\n", (int)lifo_size(&tmp));
    tmp_c = 'n',lifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'i',lifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'h',lifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'a',lifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'o',lifo_enqueue(&tmp, &tmp_c);
    printf("this lifo size:%d\n", (int)lifo_size(&tmp));
    if(!lifo_is_not_full(&tmp))
    {
        printf("this lifo is full\n");
    }
    while(lifo_is_not_empty(&tmp))
    {
        lifo_dequeue(&tmp, &tmp_c);
        printf("this char is :%c\n", tmp_c);
    }
    lifo_clear(&tmp);
    lifo_free(&tmp);
}
void test_fifo()
{
    printf("------------------test fifo------------------\n");
    fifo_t tmp;
    char tmp_c;
    fifo_alloc(&tmp, 5, 1);
    printf("this fifo size:%d\n", (int)fifo_size(&tmp));
    tmp_c = 'n',fifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'i',fifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'h',fifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'a',fifo_enqueue(&tmp, &tmp_c);
    tmp_c = 'o',fifo_enqueue(&tmp, &tmp_c);
    printf("this fifo size:%d\n", (int)fifo_size(&tmp));
    if(!fifo_is_not_full(&tmp))
    {
        printf("this fifo is full\n");
    }
    while(fifo_is_not_empty(&tmp))
    {
        fifo_dequeue(&tmp, &tmp_c);
        printf("this char is :%c\n", tmp_c);
    }
    fifo_clear(&tmp);
    fifo_free(&tmp);
}
void test_list()
{
    printf("------------------test list------------------\n");
    list_t tmp;
    char tmp_c;
    list_init(&tmp, 1);
    printf("this list size:%d\n", (int)list_size(&tmp));
    tmp_c = 'n',list_push_back(&tmp, &tmp_c);
    tmp_c = 'i',list_push_back(&tmp, &tmp_c);
    tmp_c = 'h',list_push_back(&tmp, &tmp_c);
    tmp_c = 'a',list_push_back(&tmp, &tmp_c);
    tmp_c = 'o',list_push_back(&tmp, &tmp_c);
    printf("this list size:%d\n", (int)list_size(&tmp));
    for(int i = 0 ; i < 5; ++i)
    {
        list_pop_front(&tmp, &tmp_c);
        printf("this char is :%c\n", tmp_c);
    }
    printf("this list size:%d\n", (int)list_size(&tmp));
    list_clear(&tmp);
}
void test_iterator()
{
    printf("------------------test iterator------------------\n");
    list_t tmp;
    char tmp_c;
    list_init(&tmp, 1);
    printf("this list size:%d\n", (int)list_size(&tmp));
    tmp_c = 'n',list_push_back(&tmp, &tmp_c);
    tmp_c = 'i',list_push_back(&tmp, &tmp_c);
    tmp_c = 'h',list_push_back(&tmp, &tmp_c);
    tmp_c = 'a',list_push_back(&tmp, &tmp_c);
    tmp_c = 'o',list_push_back(&tmp, &tmp_c);
    printf("this list size:%d\n", (int)list_size(&tmp));

    for (list_lnk_t *it = iterator_start_from_head(&tmp); it; it = iterator_next(it))
    {
        iterator_get(&tmp, it, &tmp_c);
        printf("this char is :%c\n", tmp_c);
    }
    list_clear(&tmp);
    printf("this list size:%d\n", (int)list_size(&tmp));
}

int test_libcollections()
{
    test_lifo();
    test_fifo();
    test_list();
    test_iterator();
    return 0;
}
