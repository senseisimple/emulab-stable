#
# For installation.
#
INSTALLDIR	= /usr/testbed/www
INSTALL		= /usr/bin/install -c -m 644

FILES		= tbstyle.css $(wildcard *.jpg *.html *.php3)

install: $(addprefix $(INSTALLDIR)/, $(FILES))

$(INSTALLDIR)/%: %
	@echo Installing $@
	-mkdir -p $(patsubst %/,%,$(dir $@))
	$(INSTALL) $< $@
