include ../../variables.mk

PORTMAP_VERSION	=	6.0
PORTMAP_PATH	=	$(TARGET_BUILD_PATH)/portmap-$(PORTMAP_VERSION)

.PHONY: extract patch config \
	portmap install clean all

all: portmap

extract: $(PORTMAP_PATH)/.extract-stamp

patch: $(PORTMAP_PATH)/.patch-stamp

config: $(PORTMAP_PATH)/.config-stamp

portmap: $(PORTMAP_PATH)/portmap

$(PORTMAP_PATH)/portmap: config
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
		NO_TCP_WRAPPER=true \
	        $(MAKE) -C $(PORTMAP_PATH) \
		LDFLAGS=-fPIE
		$(TARGET_CONFIGURE_OPTS) LD=x86_64-linux-uclibc-gcc

install: $(PORTMAP_PATH)/portmap
	install -m 755 $(PORTMAP_PATH)/portmap $(TARGET_PATH)/sbin
	install -m 755 $(PORTMAP_PATH)/pmap_dump $(TARGET_PATH)/sbin
	install -m 755 $(PORTMAP_PATH)/pmap_set $(TARGET_PATH)/sbin
	install -m 755 $(SOURCE_PATH)/portmap/S45portmap \
		$(TARGET_PATH)/etc/init.d
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/portmap
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/pmap_dump
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/pmap_set

clean:
	rm -rf $(PORTMAP_PATH)

$(PORTMAP_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/portmap/portmap-$(PORTMAP_VERSION).tar.gz
	touch $@

$(PORTMAP_PATH)/.patch-stamp: $(PORTMAP_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(PORTMAP_PATH) $(SOURCE_PATH)/portmap/ '*.patch'
	touch $@

$(PORTMAP_PATH)/.config-stamp: $(PORTMAP_PATH)/.patch-stamp
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \
