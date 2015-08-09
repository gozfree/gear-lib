#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "opt.h"

struct options opt;

static void show_usage();

static void
show_usage()
{
	printf("usage: swc [options]\n\
	  -h 打印该帮助信息\n\
	  -v 显示程序内部提示\n");
}

void
options_init()
{
	opt.verbose = 0;
	opt.width = 640;
	opt.height = 480;
}

void
options_deal(int argc, char *argv[])
{
	int my_opt;

	while ((my_opt = getopt(argc, argv, "hv")) != -1) {
		switch(my_opt) {
		case 'h':
			show_usage();
			break;
		case 'v':
			opt.verbose = 1;
			break;
		default:
			show_usage();
		exit(EXIT_FAILURE);
		}
	}
}
