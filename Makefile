CC = gcc
CFLAGS = -Wall -std=gnu99 -fPIC -I./  -g0 -O2
LOADER = -shared -soname=libmihl.so
LOADMIHL = -L./ -l mihl
FUSE = `pkg-config fuse3 --cflags --libs`
COMP := $(filter-out mihlserver.c ourfs.c,$(wildcard *.c))
OBJ := $(patsubst %.c,%.o,$(filter-out mihlserver.c ourfs.c,$(wildcard *.c)))

all : main

main: $(OBJ)
	ld $(LOADER) -o libmihl.so $(OBJ)
	rm *.o
	$(CC) -o mihlserver mihlserver.c $(LOADMIHL)
	$(CC) -o ourfs ourfs.c $(FUSE)

$(OBJ):
	$(CC) $(CFLAGS) -c $*.c 