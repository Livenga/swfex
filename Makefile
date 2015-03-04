CC = gcc
FLAGS = -g -Wall -I include/ -lz -ljpeg -lpng
SOURCE = main.c\
				 swf/swfdump.c\
				 swf/inflate.c\
				 swf/swftag.c\
				 graphics/swfjpg.c\
				 graphics/convert_png.c\
				 graphics/swfbitmap.c\
				 sound/swfsound.c\
				 util/util.c

OBJC = main.o swfdump.o inflate.o util.o swftag.o swfjpg.o convert_png.o\
			 swfbitmap.o swfsound.o

PRJC = swfex

vpath %.c src/
vpath %.h include/

$(PRJC):$(OBJC)
	$(CC) -o $@ $^ $(FLAGS)
$(OBJC):$(SOURCE)
	$(CC) -c $^ $(FLAGS)
