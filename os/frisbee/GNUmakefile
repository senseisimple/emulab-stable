all:	frisbee fromage

CFLAGS = -O -g

CFROMAGEFLAGS = -O -g -static -D_THREAD_SAFE -I/usr/local/include/pthread/linuxthreads
CFRISBEEFLAGS = -O -g

FROMAGELIBS = -lz -L/usr/local/lib -llthread -llgcc_r
FRISBEELIBS = -L/usr/local/lib

fromage: fromage.o f_chunker.o f_frisbee.o f_network.o f_timer.o
	$(CC) $(CFROMAGEFLAGS) fromage.o f_chunker.o f_frisbee.o f_network.o f_timer.o $(FROMAGELIBS) -o fromage
	cp fromage fromage.debug
	strip fromage

frisbee: server.o f_network.o f_timer.o
	$(CC) $(CFRISBEEFLAGS) server.o f_network.o f_timer.o $(FRISBEELIBS) -o frisbee
	cp frisbee frisbee.debug
	strip frisbee

tidy:
	-/bin/rm -f *.o 

clean:	tidy
	-/bin/rm fromage frisbee fromage.debug frisbee.debug
