/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libmp4parser.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-27 15:03:36
 * updated: 2016-07-27 15:03:36
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "mp4.h"
#include "malloc.h"
#include "string.h"
#include "libmp4parser.h"
#include "libfile.h"

int main(int argc, char* argv[])
{
    mp4_box_t *root,*SearchResult = NULL;
    stream_t* s = NULL;
    unsigned long filesize = 0;
    BUFFER_t *buffer = NULL;
    struct file *f = file_open("test.mp4", F_RDONLY);
    filesize = file_size("test.mp4");
    buffer = (BUFFER_t *)malloc(sizeof(BUFFER_t));
    buffer->begin_addr = (unsigned char *)malloc(filesize);
    buffer->buf = (unsigned char *)malloc(filesize);

    file_read(f, buffer->begin_addr, filesize);
    file_close(f);

    memcpy(buffer->buf,buffer->begin_addr,filesize);

    (*buffer).offset = 0;
    (*buffer).filesize = filesize;
    s = create_buffer_stream();
    if (buffer_open(s, buffer) == 0)
        return -1;
    root = MP4_BoxGetRootFromBuffer(s,filesize);
    SearchResult = MP4_BoxSearchBox(root,ATOM_mmth);
    //printf("search result box is %c%c%c%c\n",SearchResult->i_type&0x000000ff,(SearchResult->i_type&0x0000ff00)>>8,(SearchResult->i_type&0x00ff0000)>>16,(SearchResult->i_type&0xff000000)>>24);
    MP4_BoxFreeFromBuffer( root );

    buffer_close(s);
    destory_buffer_stream(s);
    return 0;
}

