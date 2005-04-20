PROGRAMS=sf sflisten

all: $(PROGRAMS)

sf: sf.c serialsource.c sfsource.c platform.c platform.h serialsource.h sfsource.h
	gcc -O1 -Wall -g -o $@ sf.c serialsource.c sfsource.c platform.c

# For ARM
asf: sf.c serialsource.c sfsource.c platform.c platform.h serialsource.h sfsource.h
	arm-linux-gcc -O1 -Wall -g -o $@ sf.c serialsource.c sfsource.c platform.c

sflisten: sflisten.c sfsource.c platform.c platform.h sfsource.h
	gcc -O1 -Wall -g -o $@ sflisten.c sfsource.c platform.c

asflisten: sflisten.c sfsource.c platform.c platform.h sfsource.h
	arm-linux-gcc -O1 -Wall -g -o $@ sflisten.c sfsource.c platform.c

aseriallisten: seriallisten.c sfsource.c platform.c platform.h sfsource.h
	arm-linux-gcc -O1 -Wall -g -o $@ seriallisten.c serialsource.c platform.c

rssi_cnt_listen: rssi_cnt_listen.c sfsource.c platform.c platform.h sfsource.h
	arm-linux-gcc -O1 -Wall -g -o $@ pkt_listen.c serialsource.c platform.c

clean:
	rm -f $(PROGRAMS) asf

