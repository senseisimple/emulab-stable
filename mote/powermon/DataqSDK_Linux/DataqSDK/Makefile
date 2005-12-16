#     author               : Ioan S. Popescu
#
# Copyright (C) 2004 DATAQ Instruments, Inc. <develop@dataq.com>
#
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either 
# version 2 of the License, or (at your option) any later 
# version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

CC = g++
# unless you are using a version of gcc >=3.5, don't use optimization
CFLAGS = -Wall -c -O0 -o
#CFLAGS = -Wall -g -c -O0 -o
TEMP_OBJ_FILES = dsdk/dsdk.o \
                 di_serial_io/di_serial_io.o \
                 di194/di194.o \
                 di194/di194_commands.o \
                 di154/di154.o
OBJ_FILES = dataqsdk.o \
            $(TEMP_OBJ_FILES)
DATAQSDK_DEPS = dataqsdk.h \
                dataqsdk.cpp \
                $(TEMP_OBJ_FILES)
INS_PATH = /usr/local/lib/

default : $(OBJ_FILES)
shared : $(OBJ_FILES)
	g++ -shared -Wl,-soname,libdataqsdk.so.0 -o libdataqsdk.so.0.0 \
	$(OBJ_FILES)
static : $(OBJ_FILES)
	ar crsu libdataqsdk.a $(OBJ_FILES)
dataqsdk.o : $(DATAQSDK_DEPS) 
	$(CC) $(CFLAGS) dataqsdk.o dataqsdk.cpp
dsdk/dsdk.o : dsdk/dsdk.h dsdk/dsdk.cpp
	$(CC) $(CFLAGS) dsdk/dsdk.o dsdk/dsdk.cpp
di_serial_io/di_serial_io.o : di_serial_io/di_serial_io.h di_serial_io/di_serial_io.cpp
	$(CC) $(CFLAGS) di_serial_io/di_serial_io.o di_serial_io/di_serial_io.cpp
di194/di194.o : di194/di194.h di194/di194.cpp di194/di194_commands.o di_serial_io/di_serial_io.o
	$(CC) $(CFLAGS) di194/di194.o di194/di194.cpp
di194/di194_commands.o : di194/di194_commands.h di194/di194_commands.cpp di_serial_io/di_serial_io.o
	$(CC) $(CFLAGS) di194/di194_commands.o di194/di194_commands.cpp
di154/di154.o : di154/di154.h di154/di154.cpp di194/di194.o di194/di194_commands.o di_serial_io/di_serial_io.o
	$(CC) $(CFLAGS) di154/di154.o di154/di154.cpp
clean :
	rm -f dataqsdk.o libdataqsdk.*
clean-all :
	rm -f libdataqsdk.*
	rm -f $(OBJ_FILES)
install : libdataqsdk.so.0.0
	mv -f libdataqsdk.so.0.0 $(INS_PATH)
	chown root.root $(INS_PATH)libdataqsdk.so.0.0
	chmod og-w $(INS_PATH)libdataqsdk.so.0.0
	/sbin/ldconfig
	ln -sf $(INS_PATH)libdataqsdk.so.0 $(INS_PATH)libdataqsdk.so
uninstall :
	rm -f $(INS_PATH)libdataqsdk.*
