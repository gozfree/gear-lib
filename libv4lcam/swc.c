#include <stdlib.h>
#include "opt.h"
#include "video.h"
#include "screen.h"

int
main(int argc, char *argv[])
{
	options_init();
	options_deal(argc, argv);
	video_init();
	screen_init();
	screen_mainloop();
	screen_quit();
	video_quit();
	exit(EXIT_SUCCESS);
}
