PROGRAMS = serial_listen sf_listen
TOOLSDIR = ../../../../tools/src/sf

CFLAGS = -I../../../../tools/src/sf

all: $(PROGRAMS)

# this builds a copy of the c serial forwarder in this directory.
sf:	$(TOOLSDIR)/sf.c $(TOOLSDIR)/serialsource.c $(TOOLSDIR)/sfsource.c \
	$(TOOLSDIR)/platform.c $(TOOLSDIR)/platform.h \
	$(TOOLSDIR)/serialsource.h $(TOOLSDIR)/sfsource.h
	gcc -O1 -Wall -g -o $@ $(TOOLSDIR)/sf.c $(TOOLSDIR)/serialsource.c \
		$(TOOLSDIR)/sfsource.c platform.c $(CFLAGS)

# this listens to a serial forwarder
sf_listen: sf_listen.c $(TOOLSDIR)/sfsource.c $(TOOLSDIR)/platform.c \
	$(TOOLSDIR)/platform.h $(TOOLSDIR)/sfsource.h
	gcc -O1 -Wall -g -o $@ sf_listen.c $(TOOLSDIR)/sfsource.c \
		$(TOOLSDIR)/platform.c $(TOOLSDIR)/serialsource.c $(CFLAGS)

# this listens to a raw serial device
serial_listen: serial_listen.c $(TOOLSDIR)/sfsource.c $(TOOLSDIR)/platform.c \
	$(TOOLSDIR)/platform.h $(TOOLSDIR)/sfsource.h
	gcc -O1 -Wall -g -o $@ serial_listen.c $(TOOLSDIR)/serialsource.c \
		$(TOOLSDIR)/platform.c $(CFLAGS)

clean:
	rm -f $(PROGRAMS)


