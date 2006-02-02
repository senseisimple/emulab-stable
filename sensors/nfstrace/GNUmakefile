#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
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

all: db-create

db-create:
	@rm -f ./.tmp-db-create.sql ./.tmp-create-diff
	@sudo -u nfstrace -H mysqldump -d nfsdb > ./.tmp-db-create.sql
	@-diff ./nfsdb-create.sql ./.tmp-db-create.sql > ./.tmp-create-diff
	@if test -s ./.tmp-create-diff; \
	then \
		cat ./.tmp-create-diff; \
		echo "NOTE: Don't forget to update nfsdb-migrate.txt!"; \
		mv ./.tmp-db-create.sql ./nfsdb-create.sql; \
	fi
	@rm -f ./.tmp-create-diff ./.tmp-db-create.sql
