TARGETS  = xtnofile
LIBRARY = hypviewer
FILES = main
INCDIRS = -I../../include

all: $(TARGETS)

include ../../Makefile.main

$(TARGETS): $(OBJS)
	$(CC) -o $(ARCH)/$(TARGETS) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS)

clean:	_clean
