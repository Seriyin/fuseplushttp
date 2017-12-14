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

#define WRITE_END 1
#define READ_END 0

static FILE *log;


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

static char rand_buf[PIPE_BUF];

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

	for(int i=0;i<64;i++) {
		rand_buf[i]=random()%256;
	}

	int tmp = write(STDOUT_FILENO,rand_buf,64);
	if (tmp!=64) {
		fprintf(log,"write rand : %d\n",tmp);
		exit(EXIT_FAILURE);
	}

	alarm(120);
	int err = 0;
	err = read(STDIN_FILENO,rand_buf,PIPE_BUF);
	if (err==-1)
	{
		if(errno!=EINTR) {
        	fprintf(log,"read OK err : %d", errno);
        	exit(EXIT_FAILURE);
		}
    }
    fprintf(log,"read OK : %d\n",err);
	
	
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
	rand_buf[0]='\0';
}


int main(int argc, char *argv[])
{
	log = fopen("./logfs.txt","w");
	int result;
	char noise[4096];
	int pipe_to_child[2];
	int pipe_from_child[2];
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	if (pipe2(pipe_from_child,O_DIRECT) == -1) {
        perror("pipe creation failed\n");
        exit(EXIT_FAILURE);
    }

	if (pipe2(pipe_to_child,O_DIRECT) == -1) {
        perror("pipe creation failed\n");
        exit(EXIT_FAILURE);
	}
	
	initstate(time(NULL),noise,4096);

	int child_id=fork();
	if (child_id==-1) 
	{
        perror("failed creating HTTP server");
        exit(EXIT_FAILURE);
	}

	if (child_id==0) 
	{
		close(pipe_to_child[WRITE_END]);
		close(pipe_from_child[READ_END]);
		execlp("./mihlserver","mihlserver",NULL);
	}
	else {
	dup2(pipe_from_child[READ_END],STDIN_FILENO);
	dup2(pipe_to_child[WRITE_END],STDOUT_FILENO);
	close(pipe_from_child[WRITE_END]);
	close(pipe_to_child[READ_END]);

	if(signal(SIGALRM,donothing)==SIG_ERR) {
		perror("Failure registering signal\n");
		exit(EXIT_FAILURE);
	}	

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

	result = fuse_main(args.argc, args.argv, &our_oper, NULL);
	kill(child_id,SIGKILL);

	return result;

	}
}
