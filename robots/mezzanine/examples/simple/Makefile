###########################################################################
# Desc: Makefile for simple tracking example
# Author: Andrew Howard
# Date: Apr 10 2002
# CVS: $Id: Makefile,v 1.1 2004-12-12 23:36:33 johnsond Exp $
###########################################################################

.SUFFIXES: .c .o

###########################################################################
# Options
###########################################################################

EXE = simple
OBS = simple.o

INCLUDES = -I../../libmezz
CFLAGS = -g3 -Wall $(INCLUDES)
LFLAGS = -lm -L../../libmezz -lmezz

CC = gcc
LINKER = gcc
MAKEDEP = makedepend


###########################################################################
# Build section
###########################################################################

all: $(EXE)

$(EXE) : $(OBS)
	$(LINKER) $(OBS) $(LFLAGS) -o $(EXE)

%.o : %.c
	${CC} ${CFLAGS} -c $< -o $@

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# Dependancies (generated by 'make dep')" *.c 2>/dev/null
	$(RM) -f Makefile.bak

clean:
	rm -f $(EXE) *~ *.o


# Dependancies (generated by 'make dep')

simple.o: ../../libmezz/mezz.h
