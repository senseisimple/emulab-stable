PROGS = cli serv

all:	${PROGS}

cli:
	gcc -g -c -Wall cli.c
	gcc -g -c -Wall get_ifi_info.c
	gcc -g -c -Wall get_rtaddrs.c
	gcc -g -c -Wall net_rt_iflist.c
	gcc -g -c -Wall sock_ntop.c
	gcc -g -c -Wall util.c
	gcc -g -Wall -o cli cli.o get_ifi_info.o \
		util.o sock_ntop.o net_rt_iflist.o \
		get_rtaddrs.o

serv:
	gcc -g -c -Wall serv_listen.c
	gcc -g -c -Wall sock_ntop.c
	gcc -g -c -Wall recvfromflags.c
	gcc -g -c -Wall if_indextoname.c
	gcc -g -c -Wall get_ifi_info.c
	gcc -g -c -Wall get_rtaddrs.c
	gcc -g -c -Wall net_rt_iflist.c
	gcc -g -c -Wall recvfromflags.c
	gcc -g -c -Wall reply.c
	gcc -g -c -Wall serv.c
	gcc -g -c -Wall util.c
	gcc -g -c -Wall forw_requests.c
	gcc -o  serv serv.o serv_listen.o sock_ntop.o \
		get_rtaddrs.o get_ifi_info.o util.o \
		net_rt_iflist.o if_indextoname.o \
		reply.o forw_requests.o recvfromflags.o

