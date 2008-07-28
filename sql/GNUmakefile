#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005, 2008 University of Utah and the Flux Group.
# All rights reserved.
#
#
# This makefile is different than all the others...
# 
# It is intended to be run in place in the cvs tree to update the files,
# and doesn't install or build anything else. This is why it doesn't go
# through configure, etc.
#

# The easy way: do 'gmake all'
# You can also run the individual targets too.

all: db-create db-fill sitevars

db-create:
	@rm -f ./.tmp-db-create.sql ./.tmp-create-diff
	@mysqldump -d tbdb > ./.tmp-db-create.sql
	@-diff ./database-create.sql ./.tmp-db-create.sql > ./.tmp-create-diff
	@if test -s ./.tmp-create-diff; \
	then \
		cat ./.tmp-create-diff; \
		echo "NOTE: Don't forget to update database-migrate.txt!"; \
		mv ./.tmp-db-create.sql ./database-create.sql; \
	fi
	@rm -f ./.tmp-create-diff ./.tmp-db-create.sql

db-fill:
	@/usr/testbed/sbin/wap /usr/testbed/sbin/export_tables \
	  > database-fill.sql

db-fill-dist: knowlbase
	@/usr/testbed/sbin/wap /usr/testbed/sbin/export_tables -i \
	  > database-fill.sql

sitevars:
	@mysqldump --skip-opt --skip-quote-names -t tbdb sitevariables > sitevars-create.sql 

knowlbase:
	@mysqldump -c -t tbdb knowledge_base_entries | sed -e 's,^INSERT,REPLACE,' > knowlbase-create.sql 

fwrules:
	@echo "*** Use 'gmake insertrules' in the firewall subdir to do this"

dist:	db-fill-dist
