#
# EMULAB-COPYRIGHT
# Copyright (c) 2004-2006 University of Utah and the Flux Group.
# All rights reserved.
#

JRPCGEN_JAR=jrpcgen.jar

mtp_java.x: mtp.x
	gcc -DJAVA_RPC -E -P -x c -o $@ $<

mtp.java: mtp_java.x
	java -jar $(JRPCGEN_JAR) -p net.emulab -nobackup $<
