CPPFLAGS= -D_THREAD_SAFE -DEMULAB
CPPFLAGS_FREEBSD= $(CPPFLAGS) -I/usr/local/include/pthread/linuxthreads
CPPFLAGS_LINUX= $(CPPFLAGS) -I/usr/include/pcap 
#CFLAGS=-pedantic -ansi -Wall -Werror
CFLAGS=-Wall
LDFLAGS= -lpcap -llthread -llgcc_r -L/usr/local/lib

all: pcapper

clean:
	-rm pcapper pcapper.linux

pcapper: pcapper.c GNUmakefile
	$(CC) $(CFLAGS) $(CPPFLAGS_FREEBSD) $(LDFLAGS) pcapper.c \
		-o pcapper -lpcap

pcapper.linux: pcapper.c GNUmakefile
	$(CC) $(CFLAGS) $(CPPFLAGS_LINUX) -L/usr/lib -lpthread -lpcap \
	pcapper.c -o pcapper.linux -lpcap
