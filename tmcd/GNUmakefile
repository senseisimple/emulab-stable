#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

PROGRAMS = tmcd tmcc

all:	$(PROGRAMS)

tmcd: tmcd.c
	$(CC) $(CFLAGS) -g -o tmcd tmcd.c \
		-I/usr/local/include \
		$(LFLAGS) -L/usr/local/lib/mysql -lmysqlclient

tmcc: tmcc.c
	$(CC) $(CFLAGS) -g -o tmcc tmcc.c $(LFLAGS)

tmcc.c:	decls.h
tmcd.c:	decls.h

install:	all

install:	$(INSTALL_BINDIR)/tmcd/tmcd \
		$(INSTALL_BINDIR)/tmcd/tmcd.restart

$(INSTALL_BINDIR)/tmcd/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_BINDIR)/tmcd
	$(INSTALL_PROGRAM) $< $@

clean: 
	rm -f *.o core $(PROGRAMS)

