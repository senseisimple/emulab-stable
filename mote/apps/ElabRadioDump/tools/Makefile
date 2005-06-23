# stolen pretty much from tos

all: all-recursive

install: all install-recursive

clean: clean-recursive

all-recursive install-recursive clean-recursive:
	target=`echo $@ | sed s/-recursive//`; \
	(cd src && $(MAKE) $$target)
