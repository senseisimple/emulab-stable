# Makefile for hypviewer library

LIBRARY = hypviewer
FILES = HypGraph HypGroupArray HypLink HypNode HypNodeArray HypPoint \
	HypQuat HypTransform HypView HypViewer \
	VkHypViewer


INCDIRS = -I../include
LDLIBS  =  -lm

all: $(LIBRARY)

include ../Makefile.main

$(LIBRARY) :	$(OBJS) _library


install:: $(LIBRARY) _install


clean::	_clean


