/* handler.c: HTTP Request Handlers */

#include "spidey.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Internal Declarations */
Status handle_browse_request(Request *request);
Status handle_file_request(Request *request);
Status handle_cgi_request(Request *request);
Status handle_error(Request *request, Status status);

/**
 * Handle HTTP Request.
 *
 * @param   r           HTTP Request structure
 * @return  Status of the HTTP request.
 *
 * This parses a request, determines the request path, determines the request
 * type, and then dispatches to the appropriate handler type.
 *
 * On error, handle_error should be used with an appropriate HTTP status code.
 **/
Status  handle_request(Request *r) {
    Status result;

    /* Parse request */
    debug("Parsing request");
    int status = parse_request(r);
    if(status < 0) {
        debug("Unable to parse request: %s", strerror(errno));
        return handle_error(r, HTTP_STATUS_BAD_REQUEST);
    }
    
    /* Determine request path */
    debug("Determining request path...");
    r->path = determine_request_path(r->uri);
    if(r->path == NULL) {
        debug("Unable to determine path: %s", strerror(errno));
        return handle_error(r, HTTP_STATUS_NOT_FOUND);
    }
    debug("HTTP REQUEST PATH: %s", r->path);


    /* Dispatch to appropriate request handler type based on file type */ 
    struct stat s;
    if( lstat(r->path, &s) < 0) {
        debug("Unable to get file information: %s", strerror(errno));
        return handle_error(r, HTTP_STATUS_NOT_FOUND);
    }


    if(S_ISDIR(s.st_mode)){
        debug("Input type: Directory");
        result = handle_browse_request(r);
    }
    else if(access(r->path, X_OK) == 0){
        debug("Input type: CGI");
        result = handle_cgi_request(r);
    }
    else if(access(r->path, R_OK) == 0){
        debug("Input type: File");
        result = handle_file_request(r);
    }
    else {
        debug("Input type: Bad --> ERROR");
        result = HTTP_STATUS_BAD_REQUEST;
    }

    log("HTTP REQUEST STATUS: %s", http_status_string(result));

    return result;
}

/**
 * Handle browse request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP browse request.
 *
 u* This lists the contents of a directory in HTML.
 *
 * If the path cannot be opened or scanned as a directory, then handle error
 * with HTTP_STATUS_NOT_FOUND.
 **/
Status  handle_browse_request(Request *r) {
    struct dirent **entries;

    /* Open a directory for reading or scanning */
    int n = scandir(r->path, &entries, NULL, alphasort);
    if(n < 0) {
        debug("Error opening directory: %s", strerror(errno));
        return HTTP_STATUS_BAD_REQUEST;
    }
    debug("Scanned directory");

    /* Write HTTP Header with OK Status and text/html Content-Type */
    debug("HTTP Header...Status: OK  |  Content Type: text/html");
    fprintf(r->file, "HTTP/1.0 200 OK\n");
    fprintf(r->file, "Content-Type: text/html\n");
    fprintf(r->file, "\r\n");


    /* For each entry in directory, emit HTML list item */
    debug("Print directories to stream");
    fprintf(r->file, "<ul type=\"square\">");
    for(int i = 1; i < n; i++) {
        if(strcmp( &r->uri[strlen(r->uri) - 1], "/")) {
            fprintf(r->file, "<li><a href=\"%s/%s\">%s</a></li>\n", r->uri, entries[i]->d_name, entries[i]->d_name);
        } 
        else {
            fprintf(r->file, "<li><a href=\"%s%s\">%s</a></li>\n", r->uri, entries[i]->d_name, entries[i]->d_name);
        }
    }
    fprintf(r->file, "</ul>");


    debug("Free directory entries");
    for(int i = 0; i < n; i++) {
        free(entries[i]);
    }
    free(entries);

    /* Flush socket, return OK */
    debug("Flush socket and return OK");
    fflush(r->file);
    return HTTP_STATUS_OK;
}

/**
 * Handle file request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This opens and streams the contents of the specified file to the socket.
 *
 * If the path cannot be opened for reading, then handle error with
 * HTTP_STATUS_NOT_FOUND.
 **/
Status  handle_file_request(Request *r) {
    FILE *fs;
    char buffer[BUFSIZ];
    char *mimetype;
    size_t nread;

    /* Open file for reading */
    fs = fopen(r->path, "r");
    if(!fs) {
        debug("Unable to open: %s", strerror(errno));
        goto fail;
    }
    /* Determine mimetype */
    mimetype = determine_mimetype(r->path);
    if(!mimetype){
        mimetype = DefaultMimeType;
        debug("Mimetype set to Default");
    }
    debug("Mimetype: %s", mimetype);

    /* Write HTTP Headers with OK status and determined Content-Type */
    debug("Write HTTP Header with OK status and MIMETYPE content type");
    fprintf(r->file, "HTTP/1.0 200 OK\r\n");
    fprintf(r->file, "Content-Type: %s\r\n", mimetype);
    fprintf(r->file, "\r\n");

    
    /* Read from file and write to socket in chunks */
    while(0 < (nread = fread(buffer, 1, BUFSIZ, fs))) {
        if( (fwrite(buffer, 1, nread, r->file)) != nread){
            debug("Failure reading and writing socket: %s", strerror(errno));
            goto fail;
        }
    }
    

    /* Close file, flush socket, deallocate mimetype, return OK */
    debug("Closing, flushing, freeing, OK");
    fclose(fs);
    fflush(r->file);
    free(mimetype);

    return HTTP_STATUS_OK;

fail:
    /* Close file, free mimetype, return INTERNAL_SERVER_ERROR */
    debug("Failed: exiting with internal server error");
    fclose(fs);
    free(mimetype);
    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
}


/**
 * Handle CGI request
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This popens and streams the results of the specified executables to the
 * socket.
 *
 * If the path cannot be popened, then handle error with
 * HTTP_STATUS_INTERNAL_SERVER_ERROR.
 **/
Status  handle_cgi_request(Request *r) {
    FILE *pfs;
    char buffer[BUFSIZ]; 

    /* Export CGI environment variables from request:
     * http://en.wikipedia.org/wiki/Common_Gateway_Interface */
    debug("Setting initial enviromental variables...");
    setenv("DOCUMENT_ROOT", RootPath, true);
    setenv("QUERY_STRING", r->query, true);
    setenv("REMOTE_ADDR", r->host, true);
    setenv("REMOTE_PORT", r->port, true);
    setenv("REQUEST_METHOD", r->method, true);
    setenv("REQUEST_URI", r->uri, true);
    setenv("SCRIPT_FILENAME", r->path, true);
    setenv("SERVER_PORT", Port, true);
    

    /* Export CGI environment variables from request headers */
    for(Header *temp = r->headers; temp; temp = temp->next) {
        if(streq(temp->name, "Host")){
            setenv("HTTP_HOST", temp->value, true);
        }
        else if(streq(temp->name, "Connection")){
            setenv("HTTP_CONNECTION", temp->value, true);
        }
        else if(streq(temp->name, "Accept")) {
            setenv("HTTP_ACCEPT", temp->value, true);
        }
        else if(streq(temp->name, "Accept-Encoding")){
            setenv("HTTP_ACCEPT_ENCODING", temp->value, true);
        }
        else if(streq(temp->name, "Accept-Language")) {
            setenv("HTTP_ACCEPT_LANGUAGE", temp->value, true);
        }
        else if(streq(temp->name, "User-Agent")) {
            setenv("HTTP_USER_AGENT", temp->value, true);
        }

                
    }
    debug("All enviromental variables set");

    /* POpen CGI Script */
    log("Executing CGI Script: %s", r->path);
    pfs = popen(r->path, "r");
    if(!pfs) {
        debug("CGI script not found");
        return HTTP_STATUS_NOT_FOUND;
    }

    /* Copy data from popen to socket */
    debug("Copying popen to socket");
    while(fgets(buffer, BUFSIZ, pfs)) {
        fputs(buffer, r->file);
    }



    /* Close popen, flush socket, return OK */
    debug("Closing, flushing, OK");
    pclose(pfs);
    fflush(r->file);
    return HTTP_STATUS_OK;
} 

/**
 * Handle displaying error page
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP error request.
 *
 * This writes an HTTP status error code and then generates an HTML message to
 * notify the user of the error.
 **/
Status  handle_error(Request *r, Status status) {
    const char *status_string = http_status_string(status);

    /* Write HTTP Header */
    debug("ERROR has occurred");
    debug("Error Status String: %s", status_string);
    fprintf(r->file, "HTTP/1.0 %s\n", status_string);
    fprintf(r->file, "Content-Type: text/html\n");
    fprintf(r->file, "\r\n");


    /* Write HTML Description of Error*/
    char *terminator = "https://www.thewrap.com/wp-content/uploads/2017/09/terminator-timeline.jpg";
    fprintf(r->file,"<h1>%s</h1>\n", status_string);
    fprintf(r->file,"<h1>Hasta la vista, baby</h2>\n");
    fprintf(r->file,"<center>\n");
    fprintf(r->file,"<img src=\"%s\">\n", terminator);
    fprintf(r->file,"</center>\r\n");

    /* Return specified status */
    debug("Flushing and returning");
    fflush(r->file);
    return status;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */

