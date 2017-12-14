#define _GNU_SOURCE

#ifdef __FUSE__
#define FUSE_USE_VERSION 31

#include <fuse.h>
#endif


#ifdef __MICROHTTP__

#ifndef _WIN32
#include <sys/select.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <limits.h>
#include <arpa/inet.h>

#define WRITE_END 1
#define READ_END 0
#define RAND_SIZE 128