include ../../Makeconf
include ../../GNUmakerules

CFLAGS = -O2 -g
LIBS = -lz

all:	imagezip imageunzip

imagezip: imagezip.c
	$(CC) $(CFLAGS) imagezip.c $(LIBS) -o imagezip

imageunzip: imageunzip.c
	$(CC) $(CFLAGS) imageunzip.c $(LIBS) -o imageunzip

install: $(INSTALL_BINDIR)/imagezip $(INSTALL_BINDIR)/imageunzip

clean:
	/bin/rm -f *.o imagezip imageunzip
