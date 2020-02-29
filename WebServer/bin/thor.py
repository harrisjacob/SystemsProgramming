#!/usr/bin/env python3

#import necessary modules
import multiprocessing
import os
import requests
import sys
import time

# Globals
PROCESSES = 1
REQUESTS  = 1
VERBOSE   = False
URL       = None

# Functions

#Usage function which describes the program
def usage(status):
    print('''Usage: {} [-p PROCESSES -r REQUESTS -v] URL
    -h              Display help message
    -v              Display verbose output

    -p  PROCESSES   Number of processes to utilize (1)
    -r  REQUESTS    Number of requests per process (1)
    '''.format(os.path.basename(sys.argv[0])))

    sys.exit(status)

#Performs the requests given a PID
def do_request(pid):
    ''' Perform REQUESTS HTTP requests and return the average elapsed time. '''
    start = time.time()                             #Begin elapsed time
    totElapsed = 0
    for i in range(REQUESTS):                       #Loop through number of requests
        response = requests.get(URL)                #GET the html data
        end = time.time()                           #End elapsed time
        elapsed = round(end - start, 2)             
        totElapsed += elapsed                       #Add to total elapsed time
        #print("Printing response")
        if VERBOSE:                                 #Verbose flag to display raw HTML
            print(response.text)
                                                    #Display request average time
        print('Process: {}, Request: {}, Elapsed Time: {}'.format(pid, i, elapsed))

    average = totElapsed / REQUESTS                 #Display average time for all requests
    print('Process: {}, AVERAGE:  , Elapsed Time: {}'.format(pid, average)) 
    return average

# Main execution

if __name__ == '__main__':
    # Parse command line arguments
    args = sys.argv[1:]                             #Store and check for arguments
    if(len(args) == 0):
        usage(1)

    while len(args) and args[0].startswith('-') and len(args[0]) > 1:
        currArg = args.pop(0)

        if currArg == '-v':
            VERBOSE = True
        elif currArg == '-p':
            PROCESSES = int(args[0])
            args.pop(0)
        elif currArg == '-r':
            REQUESTS = int(args[0])
            args.pop(0)
        elif currArg == '-h':
            usage(0)
        else:
            usage(1)

    if args[0]:                  #Confirm URL is present
        URL = args[0]
        #print("URL: {}".format(URL));
    else:
        usage(1)


    # Create pool of workers and perform requests
    if PROCESSES > 1:                               #If we have multiple processes...
        PID = 0
        process = range(PROCESSES)              
        pool = multiprocessing.Pool(PROCESSES)      #Create pool of processes 
                                                    #Sum the running processes as we use multiprocessing
        totElapsed  = sum(pool.map(do_request, process))    

        average = totElapsed / PROCESSES;           #Calculate and display average time elapsed
        print('TOTAL AVERAGE ELAPSED TIME: {}'.format(average))
    else:                                           #If we only have one process...
        totElapsed = do_request(0)                  #Call do_request with PID of 0 and print output
        print('TOTAL AVERAGE ELAPSED TIME: {}'.format(totElapsed))
    pass

# vim: set sts=4 sw=4 ts=8 expandtab ft=python:
