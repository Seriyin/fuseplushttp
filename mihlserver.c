#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/types.h>
#include "mihl.h"

#define TRUE 1
#define FALSE 0

static char buf[65];
static char has_read;
static char *current_host;
static pid_t parent;
 

static int http_root( mihl_cnx_t *cnx, char const *tag,
                      char const *host, void *param ) 
{
    if(current_host==NULL) 
    {
        current_host = strdup(host);        
    }
    else if (strcmp(current_host,host)) 
    {
        free(current_host);
        current_host = strdup(host);
    }
    if (buf[0]!='\0') 
    {
        kill(parent,SIGINT);
    }
    read(STDIN_FILENO,buf,65);
    has_read = TRUE;

    mihl_add( cnx, "<html>" );
    mihl_add( cnx, "<body>" );
    mihl_add( cnx, "<br>%s.<br>", buf );
    mihl_add( cnx, "<a>Use this code as the URL<a>" );
    mihl_add( cnx, "</body>" );
    mihl_add( cnx, "</html>" );
    mihl_send( cnx, NULL,
        "Content-type: text/html\r\n");

    return 0;

}

 

static int http_check_rand( mihl_cnx_t *cnx, char const *tag,
                            char const *host, void *param ) 
{
    if(current_host!=NULL) {
        if (strcmp(host,current_host)==0) {
            if(has_read==TRUE) {
                if(strcmp(tag+1,buf)==0) {
                    write(STDOUT_FILENO,"OK",3);
                    mihl_add( cnx, "<html>" );
                    mihl_add( cnx, "<body>" );
                    mihl_add(cnx,"File open OK");
                    mihl_add( cnx, "</body>" );
                    mihl_add( cnx, "</html>" );
                    mihl_send(cnx,NULL,"Content-type: text/html\r\n");
                    free(current_host);
                    current_host=NULL;
                    has_read = FALSE;
                    buf[0]='\0';
                    return 0;                    
                }
            }
        }
    }
    mihl_add( cnx, "<html>" );
    mihl_add( cnx, "<body>" );
    mihl_add( cnx, "There was an error<br>" );
    mihl_send( cnx,NULL,
        "Content-type: text/html\r\n");

    return 0;

}

 

int main( int argc, char *argv[] ) 
{
    has_read=FALSE;
    buf[0]='\0';
    mihl_ctx_t *ctx = mihl_init( NULL, 3001, 8, 
        MIHL_LOG_ERROR | MIHL_LOG_WARNING | MIHL_LOG_INFO | MIHL_LOG_INFO_VERBOSE );

    mihl_handle_get( ctx, "/", http_root, NULL );
    mihl_handle_get( ctx, NULL, http_check_rand, NULL );

    read(STDIN_FILENO,buf,8);
    parent = atoi(buf);
    buf[0]='\0';
    for (;;) 
    {

        int status = mihl_server( ctx );

        if ( status == -2 )

            break;

        sleep(2);
    }

    return 0;
}