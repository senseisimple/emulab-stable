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
	@rm -f ./.tmp-db-create.sql
	@mysqldump -d tbdb > ./.tmp-db-create.sql
	@diff ./database-create.sql ./.tmp-db-create.sql > ./.tmp-create-diff
	@if test -s ./.tmp-create-diff; \
	then \
		cat ./.tmp-create-diff; \
		echo "NOTE: Don't forget to update db-migrate.txt!"; \
		mv ./.tmp-db-create.sql ./database-create.sql; \
	fi
	@rm -f ./.tmp-create-diff
	@rm -f ./.tmp-db-create.sql

db-fill:
	@/usr/testbed/sbin/wap /usr/testbed/sbin/export_tables \
	  > database-fill.sql

sitevars:
	@mysqldump -t tbdb sitevariables > sitevars-create.sql 
