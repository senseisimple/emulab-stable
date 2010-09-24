include ../../variables.mk

JOVE_VERSION	=	4.16.0.72
JOVE_PATH	=	$(TARGET_BUILD_PATH)/jove$(JOVE_VERSION)

.PHONY: extract patch config \
	jove install clean all

all: jove

extract: $(JOVE_PATH)/.extract-stamp

patch: $(JOVE_PATH)/.patch-stamp

config: $(JOVE_PATH)/.config-stamp

jove: $(JOVE_PATH)/jjove

install: $(TARGET_PATH)/usr/bin/jove

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(JOVE_PATH) clean
	rm -f $(JOVE_PATH)/.build-stamp $(JOVE_PATH)/.config-stamp

$(JOVE_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/jove-$(JOVE_VERSION).tar.gz
	touch $@

$(JOVE_PATH)/.patch-stamp: $(JOVE_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(JOVE_PATH) $(SOURCE_PATH)/jove '*.patch'
	touch $@

$(JOVE_PATH)/.config-stamp: $(JOVE_PATH)/.patch-stamp
	cp $(SOURCE_PATH)/jove/jove.mk $(JOVE_PATH)/Makefile
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(JOVE_PATH)/jjove: $(JOVE_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
	        $(MAKE) -C $(JOVE_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=$(MFS_ARCH)-linux-uclibc-gcc \
		jjove
	touch $@

$(TARGET_PATH)/usr/bin/jove: $(JOVE_PATH)/jjove
	install -d -m 755 $(TARGET_PATH)/usr
	install -d -m 755 $(TARGET_PATH)/usr/bin
	install -m 755 $< $@
	$(STRIPCMD) --strip-unneeded $@
	touch -c $@

