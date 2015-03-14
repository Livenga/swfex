CC = gcc
FLAGS = -g -Wall -I include/ -lz -ljpeg -lpng -llzma
SOURCE = main.c\
				 swf/swfdump.c\
				 swf/swftag.c\
				 graphics/swfjpg.c\
				 graphics/convert_png.c\
				 graphics/swfbitmap.c\
				 sound/swfsound.c\
				 util/inflate.c\
				 util/util.c\
				 lzma/swflzma.c

OBJC = main.o swfdump.o inflate.o util.o swftag.o swfjpg.o convert_png.o\
			 swfbitmap.o swfsound.o\
			 swflzma.o

PRJC = swfex

vpath %.c src/
vpath %.h include/

$(PRJC):$(OBJC)
	$(CC) -o $@ $^ $(FLAGS)
$(OBJC):$(SOURCE)
	$(CC) -c $^ $(FLAGS)
