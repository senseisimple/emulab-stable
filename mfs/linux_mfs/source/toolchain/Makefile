include ../../variables.mk

BUILDROOT_CONFIG	=	$(SOURCE_PATH)/buildroot/buildroot.config.$(MFS_ARCH)
BUILDROOT_VERSION	=	20081211

.PHONY: extract patch config toolchain
.PHONY: all clean

all: toolchain

clean:
	rm -rf $(BUILDROOT_PATH)

extract: $(BUILDROOT_PATH)/.extract-stamp

patch: $(BUILDROOT_PATH)/.patch-stamp

config: $(BUILDROOT_PATH)/.config-stamp

toolchain: $(BUILDROOT_PATH)/.build-stamp

menuconfig:
		$(MAKE) -C $(BUILDROOT_PATH) menuconfig

$(BUILDROOT_PATH)/.extract-stamp:
	cd $(TOPDIR); tar xjf $(TARBALL_PATH)/buildroot-$(BUILDROOT_VERSION).tar.bz2
	touch $@

$(BUILDROOT_PATH)/.patch-stamp: $(BUILDROOT_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(BUILDROOT_PATH) $(SOURCE_PATH)/buildroot '*.patch'
	cp $(SOURCE_PATH)/toolchain/linux*.patch $(BUILDROOT_PATH)/toolchain/kernel-headers
	cp $(SOURCE_PATH)/toolchain/uClibc*.patch $(BUILDROOT_PATH)/toolchain/uClibc
	touch $@

$(BUILDROOT_PATH)/.config-stamp: $(BUILDROOT_PATH)/.patch-stamp
	cp $(BUILDROOT_CONFIG) $(BUILDROOT_PATH)/.config
	$(MAKE) -C $(BUILDROOT_PATH) oldconfig
	rmdir $(BUILDROOT_PATH)/dl || true
	ln -s $(TARBALL_PATH) $(BUILDROOT_PATH)/dl
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(BUILDROOT_PATH) oldconfig
	touch $@


$(BUILDROOT_PATH)/.build-stamp: $(BUILDROOT_PATH)/.config-stamp
	$(MAKE) -C $(BUILDROOT_PATH)
	touch $@
