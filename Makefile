# Testbed wide makefile.

.PHONY: all lib/sql.so assign/assign 

install: all 
	install -c assign/assign /usr/testbed/bin
	install -c db/avail /usr/testbed/bin
	install -c db/inuse /usr/testbed/bin
	install -c db/mac2if /usr/testbed/bin
	install -c db/nalloc /usr/testbed/bin
	install -c db/nfree /usr/testbed/bin
	install -c db/nodeip /usr/testbed/bin
	install -c db/ptopgen /usr/testbed/bin
	install -c db/showgraph /usr/testbed/bin
	install -c db/tblog /usr/testbed/bin
	install -c discvr/cli /usr/testbed/bin
	install -c discvr/serv /usr/testbed/bin
	install -c os/gethostkey.tcl /usr/testbed/bin
	install -c os/hostkey.tcl /usr/testbed/bin
	install -c os/instimage.exp /usr/testbed/bin
	install -c os/key7 /usr/testbed/bin
	install -c os/key8 /usr/testbed/bin
	install -c os/oslist.tcl /usr/testbed/bin
	install -c os/osnode2db.tcl /usr/testbed/bin
	install -c os/osnodestatus.tcl /usr/testbed/bin
	install -c os/ossane.tcl /usr/testbed/bin
	install -c os/osset.tcl /usr/testbed/bin
	install -c os/osstatus.tcl /usr/testbed/bin
	install -c os/setupmachine.sh /usr/testbed/bin
	install -c os/imagezip/imagezip /usr/testbed/bin
	install -c tbsetup/exp_accts /usr/testbed/bin
	install -c tbsetup/genptop /usr/testbed/bin
	install -c tbsetup/ifc_setup /usr/testbed/bin
	install -c tbsetup/ifc_filegen /usr/testbed/bin
	install -c tbsetup/ir2ifc /usr/testbed/bin
	install -c tbsetup/power /usr/testbed/bin
	install -c tbsetup/resetvlans.tcl /usr/testbed/bin
	install -c tbsetup/savevlans /usr/testbed/bin
	install -c tbsetup/snmpit /usr/testbed/bin
	install -c tbsetup/tbend.tcl /usr/testbed/bin
	install -c tbsetup/tbprerun.tcl /usr/testbed/bin
	install -c tbsetup/tbreport.tcl /usr/testbed/bin
	install -c tbsetup/tbrun.tcl /usr/testbed/bin
	install -c tbsetup/vpower /usr/testbed/bin
	install -c tbsetup/vsnmpit /usr/testbed/bin
	install -c tbsetup/ir/assign.tcl /usr/testbed/bin
	install -c tbsetup/ir/extract_tb.tcl /usr/testbed/bin
	install -c tbsetup/ir/handle_ip.tcl /usr/testbed/bin
	install -c tbsetup/ns2ir/parse.tcl /usr/testbed/bin


all: lib/sql.so assign/assign discvr/cli discvr/serv os/key7 os/key8 os/imagezip/imagezip

lib/sql.so:
	@$(MAKE) -C lib sql.so

assign/assign:
	@$(MAKE) -C assign assign

discvr/cli:
	@$(MAKE) -C discvr cli

discvr/serv:
	@$(MAKE) -C discvr serv

os/key7:
	@$(MAKE) -C os key7

os/key8:
	@$(MAKE) -C os key8

os/imagezip/imagezip:
	@$(MAKE) -C os/imagezip imagezip

clean:
	@$(MAKE) -C assign clean
	@$(MAKE) -C discvr clean
	@$(MAKE) -C os  clean
	@$(MAKE) -C os/imagezip clean



