# Makefile for building event system tests
#
# $Id: Makefile,v 1.4 2002-01-29 17:24:58 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -I../lib -Wall -DDEBUG
LDFLAGS		=
LIBS		= -lpthread -L../lib -levent

CFLAGS         += `elvin-config --cflags`
LIBS           += `elvin-config --libs`

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
