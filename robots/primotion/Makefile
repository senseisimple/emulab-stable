# Makefile for Garcia robot applications
#
# Dan Flickinger
#
# 2004/10/04
# 2004/12/08


HOSTPATH=/z/garcia/brainstem

# Build for Stargate:
ARCH=ARM
CC=/usr/local/arm/3.4.1/bin/arm-linux-gcc
CXX=/usr/local/arm/3.4.1/bin/arm-linux-g++
CXXFLAGS = -DaUNIX -s -static

# Build for i386:
#ARCH=i686
#CC=g++
#CFLAGS=-DaUNIX -g


CXXFLAGS += -I$(HOSTPATH)/aCommon \
	 -I$(HOSTPATH)/aUnix \
	 -I$(HOSTPATH)/aIO/aCommon \
	 -I$(HOSTPATH)/aIO/aUnix \
	 -I$(HOSTPATH)/aUI/aCommon \
	 -I$(HOSTPATH)/aUI/aUnix \
	 -I$(HOSTPATH)/aStem/aCommon \
	 -I$(HOSTPATH)/aStem/aUnix \
	 -I$(HOSTPATH)/aGarcia/aGarcia \
	 -I$(HOSTPATH)/aGarcia/aUnix \
	 -I$(HOSTPATH)/aGarcia/aCommon \
	 -I../mtp \
	 -I./

LIBRARIES =\
	-laGarcia \
	-laStem \
	-laIO \
	-laUI \
	-lpthread \
	-lm

VPATH=$(HOSTPATH)/aCommon:$(HOSTPATH)/aUnix:dgrobot:../mtp

LDFLAGS += -L$(HOSTPATH)/aRelease/aUnix/$(ARCH)/

OBJECTSREQ=grobot gcallbacks gbehaviors simplepath
OBJECTS=grobot.o gcallbacks.o gbehaviors.o simplepath.o mtp.o

all: commotion pathmotion circle gorobot gorobotc


commotion: $(OBJECTSREQ)
	$(CC) -o commotion $(OBJECTS) $(INCLUDES) $(CFLAGS) $(LDFLAGS) commotion.cc $(LIBRARIES)

pathmotion: $(OBJECTSREQ)
	$(CC) -o pathmotion $(OBJECTS) $(INCLUDES) $(CFLAGS) $(LDFLAGS) pathmotion.cc $(LIBRARIES)

circle: $(OBJECTSREQ)
	$(CC) -o circle $(OBJECTS) $(INCLUDES) $(CFLAGS) $(LDFLAGS) circle.cc $(LIBRARIES)

gorobot: $(OBJECTS)
	$(CXX) -o gorobot $(OBJECTS) $(CXXFLAGS) $(LDFLAGS) gorobot.cc $(LIBRARIES)

mtp_send: mtp.o
	$(CC) -o mtp_send $(CXXFLAGS) $(LDFLAGS) ../mtp/mtp_send.c $<

gorobotc: $(OBJECTSREQ)
	$(CC) -o gorobotc $(OBJECTS) $(INCLUDES) $(CFLAGS) $(LDFLAGS) gorobotc.cc $(LIBRARIES)

# %.o : %.c
# 	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDES)
# 
# %.oo : %.cpp
# 	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDES)

grobot:
	$(CC) -c $(INCLUDES) $(CFLAGS) dgrobot/grobot.cc

gcallbacks:
	$(CC) -c $(INCLUDES) $(CFLAGS) dgrobot/gcallbacks.cc

gbehaviors:
	$(CC) -c $(INCLUDES) $(CFLAGS) dgrobot/gbehaviors.cc

simplepath:
	$(CC) -c $(INCLUDES) $(CFLAGS) dgrobot/simplepath.cc

ROBOTS = \
	garcia1.emulab.net \
	garcia2.emulab.net \
	garcia3.emulab.net \
	garcia4.emulab.net \
	garcia5.emulab.net \
	garcia6.emulab.net

install: gorobot mtp_send
	@for host in ${ROBOTS}; do \
		echo -n "Checking $$host: "; \
		if ping -W 1 -c 1 $$host > /dev/null 2>&1; then \
			echo "up"; \
			scp -o "Protocol 1" $^ root@$$host:apps/; \
		else \
			echo "down"; \
		fi \
	done

clean : 
	rm -f *.o
	rm -f primotion
	rm -f square
	rm -f commotion
	rm -f shell
	rm -f pathmotion
	rm -f circle
	rm -f batt
	rm -f gorobot
	rm -f gorobotc
