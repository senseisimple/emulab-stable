# Makefile for building event system tests
#
# $Id: Makefile,v 1.3 2001-12-04 15:16:01 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -I../lib -Wall -DDEBUG
LDFLAGS		=
LIBS		= -L../lib -levent

CFLAGS         += `elvin-config --cflags`
LIBS           += `elvin-config --libs`

MV		= mv -f
RM		= rm -f

tests		= test-consume \
	 	  test-produce \
		  test-attr-consume \
		  test-attr-produce \
		  test-queue \
		  test-sched

%: %.c
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $< $(LIBS) $(LIBS)

default: $(tests)

test-queue: ../sched/queue.c
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) -DTEST_EVENT_QUEUES -I../sched \
	  $< $(LIBS) $(LIBS)

test-sched: ../sched/event-sched.c
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) -DTEST_SCHED -I../sched \
	  $< ../sched/queue.o $(LIBS) $(LIBS)

$(tests): ../lib/libevent.a

clean:
	$(RM) $(tests)
