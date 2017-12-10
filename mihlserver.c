#define __MICROHTTP__
#include "ourfs.h"

#define PORT 3001
#define TRUE 1
#define FALSE 0

static char buf[PIPE_BUF];
static char has_read;
static FILE *log;


static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
    char *page;
    struct MHD_Response *response;
    int ret = 0;
    if(strcmp(url,"/")==0) {
        int err = 0;
        //Return EIO for some reason??
        err = read(pipe_to_child[READ_END],buf,PIPE_BUF);
        if (err==-1) {
            fprintf(log,"read rand err : %d", errno);
            exit(EXIT_FAILURE);
        }
        fprintf(log,"read rand : %d\n",err);    
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
    else if(strcmp(url+1,buf)==0) {
        char tmp[3] = "OK";
        if(has_read==TRUE) {
            int tmpi = write(pipe_from_child[WRITE_END],tmp,3);
            if(tmpi!=3) {
                fprintf(log,"write OK : %d %d\n",tmpi, errno);
                exit(EXIT_FAILURE);
            }
            has_read = FALSE;
            buf[0]='\0';
            tmpi = asprintf(&page, "<html><body>%s</body></html>",tmp);
            response =
                MHD_create_response_from_buffer (
                     strlen (page), (void *) page, 
				     MHD_RESPMEM_PERSISTENT);
            ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
            MHD_destroy_response (response);
        }
    }
    else {
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


int main ()
{
    log = fopen("./log.txt","w");
    has_read=FALSE;
    buf[0]='\0';

    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == daemon)
        return 1;

    (void)getchar();

    MHD_stop_daemon (daemon);
    return 0;
}




 



 



