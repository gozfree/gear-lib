/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libmp4parser.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-27 15:03:36
 * updated: 2016-07-27 15:03:36
 *****************************************************************************/
#include "stdio.h"
#include "patch.h"
#include "libmp4.h"

int main(int argc, char* argv[])
{
   MP4_Box_t *root, *SearchResult;
   stream_t* s;

   if(argc < 2){
       printf("Invalid argument, useage: \n mp4parser /path/to/mp4file \n");
       return -1;
   }

   s = create_file_stream();
   if (stream_open(s, argv[1], MODE_READ) == 0){
        printf("Can not open file\n");
      return -1;
   }

   SearchResult = MP4_BoxGetRoot(s);
#if 0
   MP4_Box_t *p_ftyp = MP4_BoxGet(SearchResult, "ftyp" );

   printf("search result box is %c%c%c%c\n",SearchResult->i_type&0x000000ff,(SearchResult->i_type&0x0000ff00)>>8,(SearchResult->i_type&0x00ff0000)>>16,(SearchResult->i_type&0xff000000)>>24);
   printf("ftyp.major_brand is %s\n",(char *)&p_ftyp->data.p_ftyp->i_major_brand);

   MP4_BoxFree(s, root);
#endif
   MP4_BoxFree(s, SearchResult);

   stream_close(s);
   destory_file_stream(s);

	return 0;
}


