/* single.c: Single User HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

/**
 * Handle one HTTP request at a time.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Exit status of server (EXIT_SUCCESS).
 **/
int single_server(int sfd) {
    /* Accept and handle HTTP request */
    Request *r;
    while (true) {
        /* Accept request */
        r = accept_request(sfd);
        if(!r) {
            debug("Error accepting request: %s", strerror(errno));
            debug("Free request and return");
            free_request(r);
            return EXIT_FAILURE;
        }

        /* Handle request */
        Status status = handle_request(r);
        const char *returnStatus = http_status_string(status);
        log("Returned status: %s", returnStatus);

        /* Free request */
        free_request(r);
        debug("Reached end \n\n");
    }

    /* Close server socket */
    if( close(sfd) < 0) {
        debug("Failed to close server socket");
        return EXIT_FAILURE;
    }

    debug("Success!");
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */

