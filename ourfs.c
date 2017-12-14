/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * ## Source code ##
 * \include hello.c
 */
#define __FUSE__

#include "ourfs.h"

static FILE *log;
static int udpSocket[2];
static struct sockaddr_in serverAddr[2];
static socklen_t addr_size[2];
static char rand_buf[RAND_SIZE];


int tryrecv(void *buf, FILE *log) {
    static int it = 0;
    int err = 0;
	err = recvfrom(udpSocket[READ_END],buf,sizeof(buf[0])*RAND_SIZE,0,(struct sockaddr *)&serverAddr[READ_END], &addr_size[READ_END]);
	if (err==-1)
	{
		if(errno!=EINTR) {
        	fprintf(log,"recv %d err : %d", it, errno);
        	exit(EXIT_FAILURE);
		}
    }
    fprintf(log,"recv %d : %d\n", it, err);
    it++;
    return err;
}

int trysend(void *buf,FILE *log) {
    int err=sendto(udpSocket[WRITE_END],buf,sizeof(buf[0])*RAND_SIZE,0,(struct sockaddr *)&serverAddr[WRITE_END],addr_size[WRITE_END]);
	if (err!=(sizeof(buf[0])*RAND_SIZE)) {
		fprintf(log,"write rand : %d\n",errno);
		exit(EXIT_FAILURE);
    }
    return err;
}

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
	const char *filename;
	const char *contents;
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("--name=%s", filename),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};


static void *our_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
	return NULL;
}

static int our_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path+1, options.filename) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(options.contents);
	} else
		res = -ENOENT;

	return res;
}

static int our_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	filler(buf, options.filename, NULL, 0, 0);

	return 0;
}

static int our_open(const char *path, struct fuse_file_info *fi)
{
	rand_buf[0]='\0';
	if (strcmp(path+1, options.filename) != 0)
		return -ENOENT;

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	for(int i=0;i<RAND_SIZE;i++) {
		rand_buf[i]=random()%256;
	}

	int err;
	err=trysend(rand_buf,log);

	for(int i=0; i<RAND_SIZE;i++) {
        rand_buf[i]='\0';
    }

	alarm(120);
	err=tryrecv(rand_buf,log);
	
	
	if(strcmp(rand_buf,"OK")==0) {
		return 0;
	}

	return -EACCES;
}

static int our_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path+1, options.filename) != 0)
		return -ENOENT;

	len = strlen(options.contents);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, options.contents + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations our_oper = {
	.init       = our_init,
	.getattr	= our_getattr,
	.readdir	= our_readdir,
	.open		= our_open,
	.read		= our_read,
};

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --name=<s>          Name of the file to control access\n"
	       "                        (default: \"access\")\n"
	       "\n");
}


static void donothing(int sig) {
	for(int i=0; i<RAND_SIZE;i++) {
        rand_buf[i]='\0';
    }
}


void init_sockets() {
	/*Create Read UDP socket*/
  	udpSocket[READ_END] = socket(PF_INET, SOCK_DGRAM, 0);
	if(udpSocket[READ_END]==-1) {
		perror("Failure creating Read UDP socket");
		exit(EXIT_FAILURE);
	}

	/*Create Write UDP socket*/
  	udpSocket[WRITE_END] = socket(PF_INET, SOCK_DGRAM, 0);
	if(udpSocket[WRITE_END]==-1) {
		perror("Failure creating Write UDP socket");
		exit(EXIT_FAILURE);
	}
	

	/*Configure settings in address struct*/
  	serverAddr[READ_END].sin_family = AF_INET;
  	serverAddr[READ_END].sin_port = htons(3003);
  	serverAddr[READ_END].sin_addr.s_addr = inet_addr("127.0.0.1");
  	memset(serverAddr[READ_END].sin_zero, '\0', sizeof serverAddr[READ_END].sin_zero);

	serverAddr[WRITE_END].sin_family = AF_INET;
  	serverAddr[WRITE_END].sin_port = htons(3002);
  	serverAddr[WRITE_END].sin_addr.s_addr = inet_addr("127.0.0.1");
  	memset(serverAddr[WRITE_END].sin_zero, '\0', sizeof serverAddr[WRITE_END].sin_zero);
	  

	if(bind(udpSocket[WRITE_END], (struct sockaddr *) &(serverAddr[WRITE_END]), sizeof serverAddr[WRITE_END])==-1)
	{
		perror("Failure binding UDP socket to port 3002");
		exit(EXIT_FAILURE);
	}
    addr_size[READ_END] = sizeof serverAddr[READ_END];
	addr_size[WRITE_END] = sizeof serverAddr[WRITE_END];

}



int main(int argc, char *argv[])
{
	log = fopen("./logfs.txt","w");
	int result;
	char noise[4096];
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if(signal(SIGALRM,donothing)==SIG_ERR) {
		perror("Failure registering signal\n");
		exit(EXIT_FAILURE);
	}	

	init_sockets();

	initstate(time(NULL),noise,4096);

	int child_id=fork();
	if (child_id==-1) 
	{
        perror("failed creating HTTP server");
        exit(EXIT_FAILURE);
	}

	if (child_id==0) 
	{
		execlp("./mihlserver","mihlserver",NULL);
	}
	else {
	
	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	options.filename = strdup("access");
	options.contents = strdup("The file was opened correctly, Congrats!\n");

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0] = (char*) "";
	}

	for(int i=0; i<RAND_SIZE;i++) {
        rand_buf[i]='\0';
    }


	result = fuse_main(args.argc, args.argv, &our_oper, NULL);
	kill(child_id,SIGKILL);

	return result;

	}
}
