include ../../variables.mk

ETHTOOL_VERSION	=	6+20091202.orig
ETHTOOL_PATH	=	$(TARGET_BUILD_PATH)/ethtool-$(ETHTOOL_VERSION)

.PHONY: extract patch config \
	ethtool install clean all

all: ethtool

extract: $(ETHTOOL_PATH)/.extract-stamp

patch: $(ETHTOOL_PATH)/.patch-stamp

config: $(ETHTOOL_PATH)/.config-stamp

ethtool: $(ETHTOOL_PATH)/ethtool

install: $(TARGET_PATH)/usr/sbin/ethtool

clean:
	rm -rf $(ETHTOOL_PATH)

$(ETHTOOL_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/ethtool-$(ETHTOOL_VERSION).tar.gz
	touch $@

$(ETHTOOL_PATH)/.patch-stamp: $(ETHTOOL_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(ETHTOOL_PATH) $(SOURCE_PATH)/ethtool/ '*.patch'
	touch $@

$(ETHTOOL_PATH)/.config-stamp: $(ETHTOOL_PATH)/.patch-stamp
	rm -f $(ETHTOOL_PATH)/config.cache
	(cd $(ETHTOOL_PATH);  \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./autogen.sh; \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./configure \
		--target=$(MFS_ARCH)-linux-uclibc \
		--host=$(MFS_ARCH)-linux-uclibc \
		--build=$(MFS_ARCH)-linux-gnu \
		--prefix=/usr \
		--exec-prefix=/usr \
		--bindir=/usr/bin \
		--sbindir=/usr/sbin \
		--libdir=/lib \
		--libexecdir=/usr/lib \
		--sysconfdir=/etc \
		--datadir=/usr/share \
		--localstatedir=/var \
		--mandir=/usr/man \
		--infodir=/usr/info \
		$(ETHTOOL_EXTRA_CONFIG) \
	)
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(ETHTOOL_PATH)/ethtool: $(ETHTOOL_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
	        $(MAKE) -C $(ETHTOOL_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=$(MFS_ARCH)-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/usr/sbin/ethtool: $(ETHTOOL_PATH)/ethtool
	install -d -m 755 $(TARGET_PATH)/usr
	install -d -m 755 $(TARGET_PATH)/usr/bin
	mkdir -p $(TARGET_PATH)/usr/sbin
	install -m 755 $(ETHTOOL_PATH)/ethtool \
		$(TARGET_PATH)/usr/sbin/ethtool
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/usr/sbin/ethtool
	touch -c $@

