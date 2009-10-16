include ../../variables.mk

PERL_VERSION	=	5.8.8
PERL_PATH	=	$(TARGET_BUILD_PATH)/perl-$(PERL_VERSION)

PERL_MODULES	=	Digest Errno Fcntl File IO PerlIO POSIX \
			Socket Sys Time XS Getopt \
			DynaLoader \
			Carp Config Cwd English Env Errno Exporter \
			FileHandle Shell Switch UNIVERSAL XSLoader \
			attributes attrs base bytes charnames constant \
			fields if integer less lib locale open sigtrap \
			sort strict subs utf8 vars warnings


.PHONY: extract patch config \
	perl install clean all

all: perl

extract: $(PERL_PATH)/.extract-stamp

patch: $(PERL_PATH)/.patch-stamp

config: $(PERL_PATH)/.config-stamp

	     #LD_LIBRARY_PATH=$(STAGING_DIR)/lib:/lib \

$(PERL_PATH)/perl: $(PERL_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(MAKE) -C $(PERL_PATH)

perl: $(PERL_PATH)/perl

install: $(PERL_PATH)/perl
	$(STRIPCMD) --strip-unneeded $(PERL_PATH)/perl
	find $(PERL_PATH)/lib -name \*.so | xargs \
		$(STRIPCMD) --strip-unneeded
	mkdir -p $(TARGET_PATH)/usr/lib/perl5/$(PERL_VERSION)
	mkdir -p $(TARGET_PATH)/usr/bin
	install -m 755 $(PERL_PATH)/perl $(TARGET_PATH)/usr/bin/perl
	$(SOURCE_PATH)/perl/install_modules $(PERL_PATH) \
		$(TARGET_PATH)/usr/lib/perl5/$(PERL_VERSION) \
		$(PERL_MODULES)

clean:
	$(MAKE) -C $(PERL_PATH) clean

$(PERL_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xjf $(TARBALL_PATH)/perl-$(PERL_VERSION).tar.bz2
	touch $@

$(PERL_PATH)/.patch-stamp: $(PERL_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(PERL_PATH) $(SOURCE_PATH)/perl/ '*.patch'
	touch $@

		#LD_LIBRARY_PATH=/lib:$(STAGING_DIR)/lib; \

$(PERL_PATH)/.config-stamp: $(PERL_PATH)/.patch-stamp
	cp $(SOURCE_PATH)/perl/config.sh.$(MFS_ARCH) $(PERL_PATH)/config.sh
	(export PATH=$(STAGING_DIR)/usr/bin:$(PATH); \
		cd $(PERL_PATH); \
		sh ./Configure -der -f config.sh && \
		sh ./makedepend)
	sed -i /command-line/d $(PERL_PATH)/makefile
	sed -i /command-line/d $(PERL_PATH)/x2p/makefile
	touch $@
