# $Id: Makefile,v 1.1 2000-10-02 21:25:36 kwright Exp $

all: mkacct rmacct

clean:
	rm -f *.8 *.gz

mkacct: 
	rm -f mkacct.8 mkacct.8.gz
	groff -t -man -Tascii mkacct.man > mkacct.8
	/usr/bin/gzip -c mkacct.8 > mkacct.8.gz

rmacct:
	rm -f rmacct.8 rmacct.8.gz
	groff -t -man -Tascii rmacct.man > rmacct.8
	/usr/bin/gzip -c rmacct.8 > rmacct.8.gz

install: 
	mv mkacct.8.gz /usr/local/man/man8  
	mv rmacct.8.gz /usr/local/man/man8  

 
