all: stubd stub-monitor dumb-client dumb-server

stubd: stubd.o stub-pcap.o lookup.o log.o
	g++ -g -Wall stubd.o stub-pcap.o lookup.o log.o -lm -lpcap -o stubd
stub-monitor: stub-monitor.c stub.h
	gcc -g -Wall stub-monitor.c -o stub-monitor
stubd.o: stubd.c stub.h log.h
	gcc -g -Wall -c stubd.c 
stub-pcap.o: stub-pcap.c stub.h log.h
	gcc -g -Wall -c stub-pcap.c
lookup.o: lookup.cc stub.h log.h
	g++ -g -Wall -c lookup.cc
log.o: log.c log.h
	gcc -g -Wall -c log.c
dumb-client: dumb-client.o
	gcc -g -Wall dumb-client.o -lm -lpcap -o dumb-client
dumb-client.o: dumb-client.c stub.h
	gcc -g -Wall -c dumb-client.c
dumb-server: dumb-server.o
	gcc -g -Wall dumb-server.o -lm -lpcap -o dumb-server
dumb-server.o: dumb-server.c stub.h
	gcc -g -Wall -c dumb-server.c

clean:
	rm *.o stubd stubm stub-monitor 
