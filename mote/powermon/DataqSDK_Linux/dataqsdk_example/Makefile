CC = g++
CFLAGS = -Wall -c

# using shared library, no libdataqsdk.a around
shared-example : main.o /usr/local/lib/libdataqsdk.so.0.0
	$(CC) -Wall -o shared-example main.o -ldataqsdk
# using static library, no libdataqsdk.so... installed
static-example : main.o libdataqsdk.a
	$(CC) -Wall -o static-example main.o -L. -ldataqsdk
main.o : main.cpp
	$(CC) $(CFLAGS) main.cpp
clean :
	rm -f shared-example static-example main.o
