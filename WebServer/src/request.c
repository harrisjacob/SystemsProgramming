/* request.c: HTTP Request Functions */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

int parse_request_method(Request *r);
int parse_request_headers(Request *r);

/**
 * Accept request from server socket.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Newly allocated Request structure.
 *
 * This function does the following:
 *
 *  1. Allocates a request struct initialized to 0.
 *  2. Initializes the headers list in the request struct.
 *  3. Accepts a client connection from the server socket.
 *  4. Looks up the client information and stores it in the request struct.
 *  5. Opens the client socket stream for the request struct.
 *  6. Returns the request struct.
 *
 * The returned request struct must be deallocated using free_request.
 **/
Request * accept_request(int sfd) {
    struct sockaddr raddr;
    socklen_t rlen = sizeof(raddr);


    /* Allocate request struct (zeroed) */
    Request *r = calloc(1, sizeof(Request));

    /* Accept a client */
    r->fd = accept(sfd, &raddr, &rlen);
    if(r->fd < 0) {
        debug("Unable to accept: %s", strerror(errno));
        goto fail;
    }
    debug("Client Accepted");


    /* Lookup client information */
    int flags = NI_NUMERICHOST | NI_NUMERICSERV;
    int status  = getnameinfo(&raddr, rlen, r->host, NI_MAXHOST, r->port, NI_MAXSERV, flags);
    if(status < 0) {
        debug("Unable to get name info: %s", gai_strerror(status));
        goto fail;
    }
    debug("Client Information...Host: %s | Port: %s", r->host, r->port);

    /* Open socket stream */
    r->file = fdopen(r->fd, "w+");
    if(!r->file) {
        debug("Unable to fdopen: %s\n", strerror(errno));
        fclose(r->file);
        goto fail;
    }
    debug("Socket stream opened");
    log("Accepted request from %s:%s", r->host, r->port);
    return r;

fail:
    /* Deallocate request struct */
    log("Failed to accept request");
    free_request(r);
    return NULL;
}

/**
 * Deallocate request struct.
 *
 * @param   r           Request structure.
 *
 * This function does the following:
 *
 *  1. Closes the request socket stream or file descriptor.
 *  2. Frees all allocated strings in request struct.
 *  3. Frees all of the headers (including any allocated fields).
 *  4. Frees request struct.
 **/
void free_request(Request *r) {
    log("Attempting to free request struct");
    
    /* Close socket or fd */
    if(!r) {
        debug("NULL request struct");
        free(r);
        return;
    }

    if( fclose(r->file) < 0) {
        close(r->fd);
        return;
    }

    /* Free allocated strings */
    debug("Free Allocated strings");
    if(r->method)
        free(r->method);
    if(r->uri) 
        free(r->uri);
    if(r->query)
        free(r->query);
    if(r->path)
        free(r->path);


    
    /* Free headers */
    debug("Free Headers");
    if(r->headers) {
        Header *curr = r->headers;
        Header *temp;
        while(curr) {
            temp = curr->next;
            if(curr->name)
                free(curr->name);

            if(curr->value)
                free(curr->value);

            free(curr);
            if(temp)
                curr = temp;
            else{
                free(temp);
                break;
            }
        }
    }
    else
        free(r->headers);

    /* Free request */
    debug("Free request struct");
    free(r);
    log("Request freed");
}

/**
 * Parse HTTP Request.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * This function first parses the request method, any query, and then the
 * headers, returning 0 on success, and -1 on error.
 **/
int parse_request(Request *r) {
   if(!r) {
       debug("NULL Request. Can not parse");
       return -1;
   }

    /* Parse HTTP Request Method */
    debug("Beginning parse request METHOD");
    int status00 = parse_request_method(r);
    if(status00 < 0) {
        debug("Unable to parse the request method: %s", strerror(errno));
        return -1;
    }

    /* Parse HTTP Request Headers*/
    debug("Beginning parse request HEADERS");
    int status01=  parse_request_headers(r);
    if(status01 < 0) {
        debug("Unable to parse request headers: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Parse HTTP Request Method and URI.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * HTTP Requests come in the form
 *
 *  <METHOD> <URI>[QUERY] HTTP/<VERSION>
 *
 * Examples:
 *
 *  GET / HTTP/1.1
 *  GET /cgi.script?q=foo HTTP/1.0
 *
 * This function extracts the method, uri, and query (if it exists).
 **/
int parse_request_method(Request *r) {
    char buffer[BUFSIZ];
    char *method;
    char *uri;
    char *query;

    /* Read line from socket */
    if(!fgets(buffer, BUFSIZ, r->file)) {
        debug("Fgets failed: %s", strerror(errno));
        goto fail;
    }
    debug("Initial input buffer: %s", buffer); 

    /* Parse method and uri */
    method = strtok(buffer, WHITESPACE);
    r->method = strdup(method);
    uri = strtok(NULL, WHITESPACE);

    query = strtok(uri, "?");
    query = strtok(NULL, WHITESPACE);

    /* Parse query from uri */
    if(query) {
        uri = strtok(uri, "?");
        r->query = strdup(query);
    } else {
        query = " ";
        r->query = strdup(query);
    }

    if(uri)
        r->uri = strdup(uri);
    else 
        goto fail;

    /* Record method, uri, and query in request struct */

    log("HTTP METHOD: %s", r->method);
    log("HTTP URI:    %s", r->uri);
    log("HTTP QUERY:  %s", r->query);

    return 0;

fail:
    log("Parse request method failed");
    return -1;
}

/**
 * Parse HTTP Request Headers.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * HTTP Headers come in the form:
 *
 *  <NAME>: <VALUE>
 *
 * Example:
 *
 *  Host: localhost:8888
 *  User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:29.0) Gecko/20100101 Firefox/29.0
 *  Accept: text/html,application/xhtml+xml
 *  Accept-Language: en-US,en;q=0.5
 *  Accept-Encoding: gzip, deflate
 *  Connection: keep-alive
 *
 * This function parses the stream from the request socket using the following
 * pseudo-code:
 *
 *  while (buffer = read_from_socket() and buffer is not empty):
 *      name, value = buffer.split(':')
 *      header      = new Header(name, value)
 *      headers.append(header)
 **/
int parse_request_headers(Request *r) {
    Header *tail;
    char buffer[BUFSIZ];
    char *name;
    char *value;

    /* Parse headers from socket */
    while( fgets(buffer, BUFSIZ, r->file) && strlen(buffer) > 2){
        Header *curr = calloc(1, sizeof(Header));

        chomp(buffer);
        debug("Current header buffer: %s", buffer);

        name = strtok(buffer,":");
        if(!name){
            free(curr);
            goto fail;
        }
        value = strtok(NULL, WHITESPACE);
        if(!value){
            free(curr);
            goto fail;
        }

        value = skip_whitespace(value); 

        curr->name = strdup(name);
        curr->value = strdup(value);

        debug("Current name: %s", curr->name);
        debug("Current value: %s", curr->value);
    
        if(!(r->headers))
            r->headers = curr;
        else
            tail->next = curr;


        tail = curr;
    }

    if(r->headers == NULL)
        goto fail;
 

#ifndef NDEBUG
   for (struct header *header = r->headers; header; header = header->next) {
        log("HTTP HEADER %s = %s", header->name, header->value);
    }
#endif
    return 0;

fail:
    log("Parse request headers failed");
    return -1;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */


