/* utils.c: spidey utilities */

#include "spidey.h"

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

/**
 * Determine mime-type from file extension.
 *
 * @param   path        Path to file.
 * @return  An allocated string containing the mime-type of the specified file.
 *
 * This function first finds the file's extension and then scans the contents
 * of the MimeTypesPath file to determine which mimetype the file has.
 *
 * The MimeTypesPath file (typically /etc/mime.types) consists of rules in the
 * following format:
 *
 *  <MIMETYPE>      <EXT1> <EXT2> ...
 *
 * This function simply checks the file extension version each extension for
 * each mimetype and returns the mimetype on the first match.
 *
 * If no extension exists or no matching mimetype is found, then return
 * DefaultMimeType.
 *
 * This function returns an allocated string that must be free'd.
 **/
char * determine_mimetype(const char *path) {
        char buffer[BUFSIZ];
        char *searchExt;
        char *ext = strrchr(path,'.')+1;
        char *ret;


        if(!ext){
            debug("Did not find extention");
            return NULL;
        }
        debug("Looking for extention: %s", ext);

        FILE *mimetypes = fopen(MimeTypesPath,"r");
        if(!mimetypes){
            fprintf(stderr, "Could not open MimeTypes with fdopen: %s\n", strerror(errno));
            return NULL;
        }

        debug("Mimetypes file was opened successfully");

        while(fgets(buffer,BUFSIZ,mimetypes)){
            chomp(buffer);
            if (!(buffer[0]) || buffer[0]=='#')continue;

            searchExt = strtok(skip_whitespace(skip_nonwhitespace(buffer)), WHITESPACE);
            if(streq(searchExt, ext)){
                debug("Found a matching extention: %s", searchExt);
                goto breaker;
            }
            while((searchExt = strtok(NULL, WHITESPACE))){
                if(streq(searchExt, ext)){
                    debug("Found a matching extention: %s", searchExt);
                    goto breaker;
                }
            }
        }
breaker:

        if(!(ret = strtok(buffer,WHITESPACE))){
            fprintf(stderr, "Could not strtok buffer: %s\n", strerror(errno));
            fclose(mimetypes);
            return NULL;
        }
        fclose(mimetypes);
        debug("Extension maps to: %s",ret);

        if(!ret) {
            ret = DefaultMimeType;
          }
        return strdup(ret);
}

/**
 * Determine actual filesystem path based on RootPath and URI.
 *
 * @param   uri         Resource path of URI.
 * @return  An allocated string containing the full path of the resource on the
 * local filesystem.
 *
 * This function uses realpath(3) to generate the realpath of the
 * file requested in the URI.
 *
 * As a security check, if the real path does not begin with the RootPath, then
 * return NULL.
 *
 * Otherwise, return a newly allocated string containing the real path.  This
 * string must later be free'd.
 **/
char * determine_request_path(const char *uri) {
    char buffer[BUFSIZ];
    char path[BUFSIZ];

    /* Attempt to copy variables */
    if( snprintf(path, BUFSIZ, "%s%s",RootPath, uri) < 0) {
        debug("Could not copy path: %s", strerror(errno));
        return NULL;
    }

    /* Get the real path */
    realpath(path, buffer);

    if(path == NULL) {
        debug("Path is null. Return");
        return NULL;
    }

    if( streq(path, RootPath) < 0){
        debug("Path and Rootpath do not equal");
        return NULL;
    }

    debug("Path: %s", path);
    return strdup(path);



}

/**
 * Return static string corresponding to HTTP Status code.
 *
 * @param   status      HTTP Status.
 * @return  Corresponding HTTP Status string (or NULL if not present).
 *
 * http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 **/
const char * http_status_string(Status status) {
    static char *StatusStrings[] = {
        "200 OK",
        "400 Bad Request",
        "404 Not Found",
        "500 Internal Server Error",
        "418 I'm A Teapot",
    };

    const char *string_status;
    switch(status) {
        case 0:
            string_status = StatusStrings[0];
            break;
        case 1:
            string_status = StatusStrings[1];
            break;
        case 2:
            string_status = StatusStrings[2];
            break;
        default:
            string_status = StatusStrings[3];
            break;
    }

    return string_status;
}

/**
 * Advance string pointer pass all nonwhitespace characters
 *
 * @param   s           String.
 * @return  Point to first whitespace character in s.
 **/
char * skip_nonwhitespace(char *s) {
    for(char *edit = s; *edit != '\0'; edit++) {
        if(*edit == ' ' || *edit == '\n' || *edit == '\t') {
            s = edit;
            break;
        }
    }
    return s;
}

/**
 * Advance string pointer pass all whitespace characters
 *
 * @param   s           String.
 * @return  Point to first non-whitespace character in s.
 **/
char * skip_whitespace(char *s) {
    for(char *edit = s; *edit != '\0'; edit++) {
        if(*edit != ' ' && *edit != '\n' && *edit != '\t') {
            s = edit;
            break;
        }
    }
    return s;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
