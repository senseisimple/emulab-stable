# Makefile for building event system
#
# $Id: Makefile,v 1.1 2001-11-01 18:48:50 imurdock Exp $

subdirs = lib sched test

default clean:
	for dir in $(subdirs); \
	do \
		( cd $$dir && make $@ ); \
	done
