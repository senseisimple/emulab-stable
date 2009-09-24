include ../../variables.mk

HDPARM_VERSION	=	8.9
HDPARM_PATH	=	$(TARGET_BUILD_PATH)/hdparm-$(HDPARM_VERSION)

.PHONY: extract patch config \
	hdparm install clean all

all: hdparm

extract: $(HDPARM_PATH)/.extract-stamp

patch: $(HDPARM_PATH)/.patch-stamp

config: $(HDPARM_PATH)/.config-stamp

hdparm: $(HDPARM_PATH)/hdparm

install: $(TARGET_PATH)/sbin/hdparm

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(HDPARM_PATH) clean
	rm -f $(HDPARM_PATH)/.build-stamp $(HDPARM_PATH)/.config-stamp

$(HDPARM_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/hdparm-$(HDPARM_VERSION).tar.gz
	touch $@

$(HDPARM_PATH)/.patch-stamp: $(HDPARM_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(HDPARM_PATH) $(SOURCE_PATH)/hdparm '*.patch'
	touch $@

$(HDPARM_PATH)/.config-stamp: $(HDPARM_PATH)/.patch-stamp
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(HDPARM_PATH)/hdparm: $(HDPARM_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
	        $(MAKE) -C $(HDPARM_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=x86_64-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/sbin/hdparm: $(HDPARM_PATH)/hdparm
	install -d -m 755 $(TARGET_PATH)/sbin
	install -m 755 $(HDPARM_PATH)/hdparm \
		$(TARGET_PATH)/sbin/hdparm
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/hdparm
	touch -c $@

