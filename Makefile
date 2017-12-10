CC = gcc
CFLAGS = -Wall -std=gnu99 -O2
MICROHTTP = -I$PATH_TO_LIBMHD_INCLUDES -L$PATH_TO_LIBMHD_LIBS -lmicrohttpd
FUSE = `pkg-config fuse3 --cflags --libs`

all : main

main: $(OBJ)
	$(CC) $(CFLAGS) -o mihlserver mihlserver.c $(MICROHTTP)
	$(CC) $(CFLAGS) -o ourfs ourfs.c $(FUSE)
