# Makefile for building event scheduler
#
# $Id: Makefile,v 1.3 2002-01-29 17:08:51 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -I../lib -Wall -DDEBUG
LDFLAGS		=
LIBS		= -lpthread -L../lib -levent

CFLAGS         += `elvin-config --cflags`
LIBS           += `elvin-config --libs`

MV		= mv -f
RM		= rm -f

program		= event-sched
source		= event-sched.c queue.c
object		= event-sched.o queue.o
header		= event-sched.h

.c.o:
	$(CC) $(CFLAGS) -c $<

default: $(program)

$(program): $(object)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(LIBS)

$(program): ../lib/libevent.a

$(object): $(header) ../lib/event.h

clean:
	$(RM) $(program) $(object)
