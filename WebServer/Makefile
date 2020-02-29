CC=     gcc
CFLAGS=     -g -Wall -Werror -std=gnu99 -Iinclude
LD=     gcc
LDFLAGS=    -L.
AR=     ar
ARFLAGS=    rcs
TARGETS=    bin/spidey

all:        $(TARGETS)

clean:
	@echo Cleaning...
	@rm -f $(TARGETS) lib/*.a src/*.o *.log *.input

.PHONY:     all test clean

# TODO: Add rules for bin/spidey, lib/libspidey.a, and any intermediate objects

lib/forking.o: src/forking.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/handler.o: src/handler.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/request.o: src/request.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/single.o: src/single.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/socket.o: src/socket.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/spidey.o: src/spidey.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/utils.o: src/utils.c
	$(CC) $(CFLAGS) -o $@ -c $^

lib/libspidey.a: lib/forking.o lib/handler.o lib/request.o lib/single.o lib/socket.o lib/utils.o
	$(AR) $(ARFLAGS) $@ $^

bin/spidey: lib/spidey.o lib/libspidey.a
	$(LD) $(LDFLAGS) -o $@ $^

