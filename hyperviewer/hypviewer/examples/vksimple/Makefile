TARGETS  = vksimple
LIBRARY = hypviewer
FILES = main
INCDIRS = -I../../include

all: $(TARGETS)

include ../../Makefile.main

LDLIBS  =  -L../../src/$(ARCH) -l$(LIBRARY)$(ARCH) \
	-lvk -lGLw -lGL -lGLU -lXm -lXt -lX11

$(TARGETS): $(OBJS)
	$(CC) -o $(ARCH)/$(TARGETS) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS)


clean:	_clean