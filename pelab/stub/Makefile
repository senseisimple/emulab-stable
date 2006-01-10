all: stubd stub-monitor 

stubd: stubd.o stub-pcap.o stub.h
	gcc -g -Wall -lm -lpcap stubd.o stub-pcap.o -o stubd
monitor-stub: monitor-stub.c stub.h
	gcc -g -Wall stub-monitor.c -o stub-monitor
stubd.o: stubd.c stub.h
	gcc -g -Wall -c stubd.c 
stub-pcap.o: stub-pcap.c stub.h
	gcc -g -Wall -c stub-pcap.c

clean:
	rm *.o stubd stub-monitor 
