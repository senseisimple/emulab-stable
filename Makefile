# Testbed wide makefile.

.PHONY: all lib/sql.so assign_hw/assign

all: lib/sql.so assign_hw/assign

lib/sql.so:
	@$(MAKE) -C lib sql.so

assign_hw/assign:
	@$(MAKE) -C assign_hw
