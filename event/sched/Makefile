# Makefile for building event scheduler
#
# $Id: Makefile,v 1.1 2001-11-02 05:40:53 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -I../lib -Wall -DDEBUG
LDFLAGS		=
LIBS		= -L../lib -levent

CFLAGS         += `elvin-config --cflags`
LIBS           += `elvin-config --libs`

MV		= mv -f
RM		= rm -f

program		= event-sched
source		= queue.c sched.c
object		= queue.o sched.o
header		= sched.h

.c.o:
	$(CC) $(CFLAGS) -c $<

default: $(program)

$(program): $(object)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(LIBS)

$(program): ../lib/libevent.a

$(object): $(header) ../lib/event.h

clean:
	$(RM) $(program) $(object)
