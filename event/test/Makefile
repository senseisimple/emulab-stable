# Makefile for building event system tests
#
# $Id: Makefile,v 1.1 2001-11-02 04:47:14 imurdock Exp $

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
		  test-attr-produce

%: %.c
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $< $(LIBS) $(LIBS)

default: $(tests)

clean:
	$(RM) $(tests) $(addsuffix .o,$(tests))

distclean: clean

$(tests): ../lib/libevent.a
