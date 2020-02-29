#include "spidey.h"

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>

/**
 * Fork incoming HTTP requests to handle the concurrently.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Exit status of server (EXIT_SUCCESS).
 *
 * The parent should accept a request and then fork off and let the child
 * handle the request.
 **/


int forking_server(int sfd) {
    /* Accept and handle HTTP request */
    while (true) {
        /* Accept request */
        Request *client_stream = accept_request(sfd);/*use free_request*/
        if(!client_stream){
            continue;
        }

        pid_t pid = fork();
        if(pid < 0){
          debug("Unable to fork: %s", strerror(errno));
          free_request(client_stream);
          continue;
        }

        /* Ignore children */
        signal(SIGCHLD, SIG_IGN);

        /* Fork off child process to handle request */
        if(pid==0){
            Status status = handle_request(client_stream);
            const char* returnStatus = http_status_string(status);
            log("Return status: %s", returnStatus);
            free_request(client_stream);
            exit(EXIT_SUCCESS);/*prevents fork bombs*/
        }else{

        }
        debug("Freeing request");
        free_request(client_stream);

    }

    /* Close server socket */
    if(close(sfd) < 0) {
        debug("Error closing server socket");
        return EXIT_FAILURE;
    }

    debug("Success!");
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
