CC = gcc
CFLAGS = -Wall -std=gnu99 -O2
PATH_TO_LIBMHD_LIBS = /usr/lib/x86_64-linux-gnu
PATH_TO_LIBMHD_INCLUDES = /usr/include/
MICROHTTP = -I$(PATH_TO_LIBMHD_INCLUDES) -L$(PATH_TO_LIBMHD_LIBS) -lmicrohttpd
FUSE = `pkg-config fuse3 --cflags --libs`

all : main

main: $(OBJ)
	$(CC) $(CFLAGS) -o mihlserver mihlserver.c $(MICROHTTP)
	$(CC) $(CFLAGS) -o ourfs ourfs.c $(FUSE)
