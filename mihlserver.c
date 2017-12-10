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

 

int http_root( mihl_cnx_t *cnx, char const *tag, char const *host, void *param ) {


    mihl_add( cnx, "<html>" );

    mihl_add( cnx, "<body>" );

    mihl_add( cnx, "This is a test HTML page for MIHL.<br>" );

    mihl_add( cnx, "<a href='nextpage.html'>Next Page<a>" );

    mihl_add( cnx, "</body>" );

    mihl_add( cnx, "</html>" );

    mihl_send( cnx, NULL,

        "Content-type: text/html\r\n");

    return 0;

}

 

int http_nextpage( mihl_cnx_t *cnx, char const *tag, char const *host, void *param ) {

    mihl_add( cnx, "<html>" );

    mihl_add( cnx, "<body>" );

    mihl_add( cnx, "This is another page...<br>" );

    mihl_add( cnx, "<a href=''>Previous Page<a>" );

    mihl_add( cnx, "</body>" );

    mihl_add( cnx, "</html>" );

    mihl_send( cnx,NULL,

        "Content-type: text/html\r\n");

    return 0;

}

 

int main( int argc, char *argv[] ) {

    mihl_ctx_t *ctx = mihl_init( NULL, 3001, 8, 

        MIHL_LOG_ERROR | MIHL_LOG_WARNING | MIHL_LOG_INFO | MIHL_LOG_INFO_VERBOSE );

 

    mihl_handle_get( ctx, "/", http_root, NULL );

    mihl_handle_get( ctx, "/nextpage.html", http_nextpage, NULL );

 

    for (;;) {

        int status = mihl_server( ctx );

        if ( status == -2 )

            break;

    }

    

    return 0;

}