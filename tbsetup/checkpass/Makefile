include ../../Makeconf
include ../../GNUmakerules

all: checkpass cracklib

checkpass: cracklib
	${CC} -g checkpass.c cracklib,2.7/cracklib/libcrack.a -o checkpass -lm

cracklib:
	gmake -C cracklib,2.7 all

install:	$(INSTALL_BINDIR)/checkpass

clean:
	gmake -C cracklib,2.7 clean
	/bin/rm -f *.o checkpass

