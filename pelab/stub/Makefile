all: stubd stub-monitor stubm

stubd: stubd.o stub-pcap.o stub.h
	gcc -g -Wall -lm -lpcap stubd.o stub-pcap.o -o stubd
stubm: stubm.o stub-pcap.o stub.h
	gcc -g -Wall -lm -lpcap stubm.o stub-pcap.o -o stubm
stub-monitor: stub-monitor.c stub.h
	gcc -g -Wall stub-monitor.c -o stub-monitor
stubd.o: stubd.c stub.h
	gcc -g -Wall -c stubd.c 
stubm.o: stubm.c stub.h
	gcc -g -Wall -c stubm.c 
stub-pcap.o: stub-pcap.c stub.h
	gcc -g -Wall -c stub-pcap.c

clean:
	rm *.o stubd stub-monitor 
