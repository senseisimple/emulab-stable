###########################################################################
#
# File: Makefile
# Author: Andrew Howard
# Date: 21 Mar 2001
# Desc:
#
# CVS info:
#  $Source: /home/cvs_mirrors/cvs-public.flux.utah.edu/CVS/testbed/robots/mezzanine/rtk2/Makefile,v $
#  $Author: johnsond $
#  $Revision: 1.1 $
#
###########################################################################

.SUFFIXES: .cc .o

# Paths
PYTHON_INC = /usr/local/include/python2.1

RTK_LIB = librtk.a
RTK_OBJS = rtk.o rtk_canvas.o rtk_fig.o rtk_menu.o rtk_table.o

PYRTK_LIB = rtk.so
PYRTK_OBJS = pyrtk.o rtk.o rtk_fig.o

# Tools
# To build for the robots, use make -e CC = i386-glibc21-linux-gcc
CC = gcc
MAKEDEP = makedepend

# Compile flags
CFLAGS = -g3 -Wall `gtk-config --cflags`

# Compile/link flags for pyrtk
PYRTK_CFLAGS = -g3 -fPIC -Wall -D_GNU_SOURCE -I$(PYTHON_INC) `gtk-config --cflags`
PYRTK_LFLAGS = -shared -lm -lpthread -L. -lrtk `gtk-config --libs gtk gthread`

###########################################################################
# Build section

all: $(RTK_LIB)

$(RTK_LIB) : $(RTK_OBJS)
	ar r $(RTK_LIB) $(RTK_OBJS)

$(PYRTK_LIB) : CFLAGS = $(PYRTK_CFLAGS)
$(PYRTK_LIB) : $(PYRTK_OBJS)
	gcc $(PYRTK_OBJS) $(PYRTK_LFLAGS) -o $(PYRTK_LIB)

%.o : %.c
	${CC} ${CFLAGS} -c $< -o $@

dep:
	$(MAKEDEP) -Y -s "# Dependancies (make dep)" *.c 2>/dev/null

clean:
	rm -f $(RTK_LIB) $(PYRTK_LIB) *~ *.o 


# Dependancies (make dep)

pyrtk.o: rtk.h
rtk.o: rtk.h rtkprivate.h
rtk_canvas.o: rtk.h rtkprivate.h
rtk_fig.o: rtk.h rtkprivate.h
rtk_menu.o: rtk.h rtkprivate.h
rtk_table.o: rtk.h rtkprivate.h
