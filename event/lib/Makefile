# Makefile for building event library
#
# $Id: Makefile,v 1.1 2001-11-02 04:43:04 imurdock Exp $

CC		= gcc
CFLAGS		= -g -I. -Wall -DDEBUG

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
