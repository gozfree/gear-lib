#ifndef SCREEN_H
#define SCREEN_H

#include <SDL/SDL.h>

struct rgb_surface {
	SDL_Surface *surface;
	unsigned int rmask;
	unsigned int gmask;
	unsigned int bmask;
	unsigned int amask;
	int width;
	int height;
	int bpp;
	int pitch;
	unsigned char *pixels;
	int pixels_num;
};

struct screen {
	SDL_Surface *display;
	SDL_Event event;
	int width;
	int height;
	int bpp;
	int running;
	struct rgb_surface rgb;
};

void screen_init();
void screen_quit();
void screen_mainloop();

#endif
