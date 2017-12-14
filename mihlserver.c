#define __MICROHTTP__
#include "ourfs.h"

#define PORT 3001
#define TRUE 1
#define FALSE 0

static char buf[RAND_SIZE];
static char has_read;
static FILE *log;
static char has_died;
static struct MHD_Daemon *d;
static int udpSocket[2];
static struct sockaddr_in serverAddr[2];
static socklen_t addr_size[2];

int tryrecv(void *buf, FILE *log) {
    static int it = 0;
    int err = 0;
	err = recvfrom(udpSocket[READ_END],buf,sizeof(buf[0])*RAND_SIZE,0,(struct sockaddr *)&(serverAddr[READ_END]), &(addr_size[READ_END]));
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

int trysend(void *buf,FILE *log,int count) {
    int err=sendto(udpSocket[WRITE_END],buf,sizeof(buf[0])*count,0,(struct sockaddr *)&(serverAddr[WRITE_END]),addr_size[WRITE_END]);
	if (err!=sizeof(buf[0])*count) {
		fprintf(log,"write rand : %d\n",errno);
		exit(EXIT_FAILURE);
    }
    return err;
}

static void stop_daemon(int sig) {
    if (d != NULL)
        MHD_stop_daemon(d);
    has_died = TRUE;
}

static void donothing(int sig) {
    for(int i=0; i<RAND_SIZE;i++) {
        buf[i]='\0';
    }
}


static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
    char *page;
    struct MHD_Response *response;
    int ret = 0;
    int err = 0;
    fprintf(log,"Serving page\n");
    if(strcmp(url,"/")==0) {
        alarm(3);
        if(tryrecv(buf,log)==-1) {
            //Alarm went off jump to standard err html;
            goto getout;
        };
        has_read = TRUE;
        err = asprintf(&page, 
            "<html><body><h1>Use this code as the URL</h1>%s</body></html>",buf);
        response =
            MHD_create_response_from_buffer (
                     strlen (page), (void *) page, 
				     MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
        MHD_destroy_response (response);
    }
    else if(memcmp(url+1,buf,RAND_SIZE)==0) {
        char tmp[4] = "OK";
        if(has_read==TRUE) {
            trysend(tmp,log,strlen(tmp));
            has_read = FALSE;
            //Clear the buffer, otherwise memcmp might be true on
            //subsequent calls.
            for(int i=0; i<RAND_SIZE;i++) {
                buf[i]='\0';
            }
            err = asprintf(&page, "<html><body>%s</body></html>",tmp);
            response =
                MHD_create_response_from_buffer (
                     strlen (page), (void *) page, 
				     MHD_RESPMEM_PERSISTENT);
            ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
            MHD_destroy_response (response);
        }
    }
    else {
        getout:
        page = "<html><body>Error</body></html>";
        response =
            MHD_create_response_from_buffer (
                     strlen (page), (void *) page, 
				     MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response (response);
    }
    
    return ret;
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
  	serverAddr[READ_END].sin_port = htons(3002);
  	serverAddr[READ_END].sin_addr.s_addr = inet_addr("127.0.0.1");
  	memset(serverAddr[READ_END].sin_zero, '\0', sizeof serverAddr[READ_END].sin_zero);

	serverAddr[WRITE_END].sin_family = AF_INET;
  	serverAddr[WRITE_END].sin_port = htons(3003);
  	serverAddr[WRITE_END].sin_addr.s_addr = inet_addr("127.0.0.1");
  	memset(serverAddr[WRITE_END].sin_zero, '\0', sizeof serverAddr[WRITE_END].sin_zero);
	  

	if(bind(udpSocket[WRITE_END], (struct sockaddr *) &(serverAddr[WRITE_END]), sizeof serverAddr[WRITE_END])==-1)
	{
		perror("Failure binding UDP socket to port 3003");
		exit(EXIT_FAILURE);
    }
    addr_size[READ_END] = sizeof (serverAddr[READ_END]);
	addr_size[WRITE_END] = sizeof (serverAddr[WRITE_END]);

}

int main ()
{
    log = fopen("./log.txt","w");
    has_read=FALSE;
    has_died = FALSE;
    d=NULL;
    for(int i=0; i<RAND_SIZE;i++) {
        buf[i]='\0';
    }

    if(signal(SIGINT,stop_daemon)==SIG_ERR){
    	perror("Failure registering signal\n");
		exit(EXIT_FAILURE);
    }

    if(signal(SIGALRM,donothing)==SIG_ERR){
		perror("Failure registering signal\n");
		exit(EXIT_FAILURE);
    }

    init_sockets();    

    d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == d)
        return 1;

    fprintf(log,"DAEMON UP\n");

    while(has_died==FALSE) 
    {
        fprintf(log,"DAEMON RUNNING\n");
        sleep(INT_MAX);
    }
    
    fprintf(log,"DAEMON DOWN\n");

    return 0;
}




 



 



