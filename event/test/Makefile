# Makefile for building event system tests
#
# $Id: Makefile,v 1.5 2002-02-19 15:51:01 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -I../lib -Wall -DDEBUG
LDFLAGS		=
LIBS		= -lpthread -L../lib -levent

CFLAGS         += `elvin-config --cflags vin4mt`
LIBS           += `elvin-config --libs vin4mt`

MV		= mv -f
RM		= rm -f

tests		= test-consume \
	 	  test-produce \
		  test-attr-consume \
		  test-attr-produce \
		  test-queue \
		  test-sched-produce

%: %.c
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $< $(LIBS) $(LIBS)

default: $(tests)

test-queue: ../sched/queue.c
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) -DTEST_EVENT_QUEUES -I../sched \
	  $< $(LIBS) $(LIBS)

$(tests): ../lib/libevent.a

clean:
	$(RM) $(tests)
