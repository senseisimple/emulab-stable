all:	userfrisbee frisbeed

CFLAGS = -O -g

CFROMAGEFLAGS = -O -g -static -D_THREAD_SAFE -I/usr/local/include/pthread/linuxthreads
CFRISBEEFLAGS = -O -g

FROMAGELIBS = -lz -L/usr/local/lib -llthread -llgcc_r
FRISBEELIBS = -L/usr/local/lib

f_network_withsyslog.c: f_network.c
	echo "#define SYSLOG" > f_network_withsyslog.c
	cat f_network.c >> f_network_withsyslog.c

userfrisbee: fromage.o f_chunker.o f_frisbee.o f_network.o f_timer.o
	$(CC) $(CFROMAGEFLAGS) fromage.o f_chunker.o f_frisbee.o f_network.o f_timer.o $(FROMAGELIBS) -o userfrisbee
	cp userfrisbee userfrisbee.debug
	strip userfrisbee

frisbeed: server.o f_network_withsyslog.o f_timer.o
	$(CC) $(CFRISBEEFLAGS) server.o f_network_withsyslog.o f_timer.o $(FRISBEELIBS) -o frisbeed
	cp frisbeed frisbeed.debug
	strip frisbeed

tidy:
	-/bin/rm -f *.o 

clean:	tidy
	-/bin/rm userfrisbee frisbeed userfrisbee.debug frisbeed.debug
