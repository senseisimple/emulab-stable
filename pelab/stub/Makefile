all: stubd stub-monitor

stubd: stubd.o stub-pcap.o lookup.o stub.h
	g++ -g -Wall stubd.o stub-pcap.o lookup.o -lm -lpcap -o stubd
stub-monitor: stub-monitor.c stub.h
	gcc -g -Wall stub-monitor.c -o stub-monitor
stubd.o: stubd.c stub.h
	gcc -g -Wall -c stubd.c 
stub-pcap.o: stub-pcap.c stub.h
	gcc -g -Wall -c stub-pcap.c
lookup.o: lookup.cc stub.h
	g++ -g -Wall -c lookup.cc

clean:
	rm *.o stubd stubm stub-monitor 
