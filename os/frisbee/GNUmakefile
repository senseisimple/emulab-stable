all:	frisbee fromage

CFROMAGEFLAGS = -O -g -static -D_THREAD_SAFE -I/usr/local/include/pthread/linuxthreads
CFRISBEEFLAGS = -O -g

LIBS = -lz -L/usr/local/lib -llthread -llgcc_r

fromage: fromage.o f_chunker.o f_frisbee.o f_network.o f_timer.o
	$(CC) $(CFROMAGEFLAGS) fromage.o f_chunker.o f_frisbee.o f_network.o f_timer.o $(LIBS) -o fromage

frisbee: server.o f_network.o f_timer.o
	$(CC) $(CFRISBEEFLAGS) server.o f_network.o f_timer.o $(LIBS) -o frisbee

tidy:
	-/bin/rm -f *.o 

clean:	tidy
	-/bin/rm fromage frisbee