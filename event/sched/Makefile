# Makefile for building event scheduler
#
# $Id: Makefile,v 1.2 2001-12-04 15:15:32 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -I../lib -Wall -DDEBUG
LDFLAGS		=
LIBS		= -L../lib -levent

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
