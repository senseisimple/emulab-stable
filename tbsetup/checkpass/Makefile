CC=/usr/gnu/bin/gcc 
# Pick one of the two below.

all: checkpass cracklib

checkpass: cracklib
	${CC} -g checkpass.c cracklib,2.7/cracklib/libcrack.a -o checkpass -lm

cracklib:
	gmake -C cracklib,2.7 all

install:
	install -c checkpass /usr/testbed/bin

clean:
	gmake -C cracklib,2.7 clean
	/bin/rm -f *.o checkpass

