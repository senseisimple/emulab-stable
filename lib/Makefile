#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

all: sql.so

sql.so:
	./mk_sqlso.sh

install: all $(INSTALL_LIBDIR)/sql.so
