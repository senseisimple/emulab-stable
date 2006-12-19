#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

SYSTEM :=	$(shell uname -s)

# I like using -pedantic, but it gives us some trouble with function pointers
CFLAGS=-Wall -Werror -std=c99

ifeq ($(SYSTEM),Linux)
    LD_FLAGS=-ldl
else
    LD_FLAGS=
endif

all: libnetmon.so netmond netmonup

%.so: %.c %.h
	$(CC) $(CFLAGS) $(LD_FLAGS) -shared -o $@ $<

clean:
	-rm *.so netmond netmonup
