OBJS=get_ifi_info.o get_rtaddrs.o net_rt_iflist.o sock_ntop.o util.o
SOBJS=serv_listen.o recvfromflags.o if_indextoname.o reply.o serv.o forw_requests.o
COBJS=cli.o 
LIBS = -lm
CFLAGS += -Wall -g 
CC=gcc

all: cli serv

cli: ${OBJS} ${COBJS}
	$(CC) $(LIBS) $(OBJS) $(COBJS) -o cli

serv: ${OBJS} ${SOBJS}
	$(CC) $(OBJS) $(SOBJS) ${LIBS} -o serv

clean: 
	/bin/rm -rf $(OBJS) $(SOBJS) $(COBJS) cli serv