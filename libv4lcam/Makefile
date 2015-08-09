#SDL library must be install
#apt-get install libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev

CC = gcc
CFLAGS = -Wall -Werror -Wcast-align -g
#LDFLAGS = -L/usr/local/lib -lSDL

LDFLAGS = -lSDL
SWCOBJECT = swc.o opt.o video.o screen.o

all: swc

swc: $(SWCOBJECT)
	$(CC) `pkg-config --libs sdl` $(SWCOBJECT) -o $@ $(LDFLAGS) 

swc.o: swc.c
	$(CC) $(CFLAGS) `pkg-config --cflags sdl` -c $<

opt.o: opt.c opt.h
	$(CC) $(CFLAGS) -c $<

video.o: video.c video.h
	$(CC) $(CFLAGS) -c $<

screen.o: screen.c screen.h
	$(CC) $(CFLAGS) `pkg-config --cflags sdl` -c $<


.PHONY: all clean
clean:
	rm -f *.o *~ swc
