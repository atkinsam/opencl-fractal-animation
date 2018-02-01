
CC=g++
OUTFILE=../render
OS := $(shell uname)
ifeq ($(OS), Darwin)
	CFLAGS=-Wall -framework OpenCL -std=c++11
else
	CFLAGS=-Wall -lOpenCL -std=c++11 -I/usr/local/cuda/include/ -L/usr/local/cuda/lib64/
endif

all: lodepng.o julia_set.o render.cpp
	$(CC) lodepng.o julia_set.o render.cpp $(CFLAGS) -o $(OUTFILE)

lodepng.o: lodepng.cpp
	$(CC) -c lodepng.cpp $(CFLAGS)

julia_set.o: julia_set.cpp
	$(CC) -c julia_set.cpp $(CFLAGS)

