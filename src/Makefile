
CC=g++
OUTFILE=../render.o
OS := $(shell uname)
ifeq ($(OS), Darwin)
	CFLAGS=-Wall -framework OpenCL -std=c++11
else
	CFLAGS=-Wall -lOpenCL -std=c++11
endif

all: lodepng.o colormap.o render.cpp
	$(CC) lodepng.o colormap.o render.cpp $(CFLAGS) -o $(OUTFILE)

lodepng.o: lodepng.cpp
	$(CC) -c lodepng.cpp

colormap.o: colormap.cpp
	$(CC) -c colormap.cpp

