# Makefile for building event library
#
# $Id: Makefile,v 1.2 2002-02-19 15:45:24 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -Wall -DDEBUG

CFLAGS         += `elvin-config --cflags vin4mt`

AR		= ar
MV		= mv -f
RANLIB		= ranlib
RM		= rm -f

library		= libevent.a
source		= event.c util.c
object		= event.o util.o
header		= event.h

.c.o:
	$(CC) $(CFLAGS) -c $<

default: $(library)

$(library): $(object)
	$(AR) crv $@ $^
	$(RANLIB) $@

$(object): $(header)

clean:
	$(RM) $(library) $(object)
