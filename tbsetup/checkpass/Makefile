CC=/usr/gnu/bin/gcc 
# Pick one of the two below.

all: checkpass cracklib

checkpass: cracklib
	${CC} -g checkpass.c cracklib,2.7/cracklib/libcrack.a -o checkpass -lm

cracklib:
	(cd cracklib,2.7/ && make all && cd ..)

install:
	install -c checkpass /usr/testbed/bin

clean:
	(cd cracklib,2.7/ && make clean && cd ..)
	/bin/rm -f *.o checkpass

