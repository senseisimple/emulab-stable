THARNESS=perl t/harness
all: usage
	$(MAKE) test

usage:
	@echo "make"
	@echo "     test"
	@echo "     exptest"
	@echo "     t/SPECIFIC_TEST.t"

test:
	$(THARNESS)

critic:
	perlcritic .

topo:
	$(THARNESS) t/topologies/*.t

# Run a single test
t/*.t t/*/*.t t/*/*/*.t: force
	$(THARNESS) $@

force: ;

