.PHONY: fakeroot-extract fakeroot-patch fakeroot-config fakeroot-build \
	fakeroot fakeroot-install fakeroot-clean

fakeroot-extract: $(FAKEROOT_PATH)/.extract-stamp

fakeroot-patch: $(FAKEROOT_PATH)/.patch-stamp

fakeroot-config: $(FAKEROOT_PATH)/.config-stamp

fakeroot-build: $(FAKEROOT_PATH)/.build-stamp

fakeroot-install: $(FAKEROOT_PATH)/.install-stamp

fakeroot-clean:
		$(MAKE) -C $(FAKEROOT_PATH) clean
		rm -f $(FAKEROOT_PATH)/.build-stamp

fakeroot: fakeroot-build

$(FAKEROOT_PATH)/.extract-stamp:
		mkdir -p $(TOOLCHAIN_BUILD_PATH)
		cd $(TOOLCHAIN_BUILD_PATH); tar xzf $(SOURCE_PATH)/fakeroot/fakeroot-$(FAKEROOT_VERSION).tar.gz
		touch $@

$(FAKEROOT_PATH)/.patch-stamp: $(FAKEROOT_PATH)/.extract-stamp
		$(SCRIPTS_PATH)/patch-kernel.sh $(FAKEROOT_PATH) $(SOURCE_PATH)/fakeroot/ '*.patch'
		touch $@

$(FAKEROOT_PATH)/.config-stamp: $(FAKEROOT_PATH)/.patch-stamp
		cd $(FAKEROOT_PATH); ./configure \
			--prefix=$(STAGING_DIR)/usr
		touch $@

$(FAKEROOT_PATH)/.build-stamp: $(FAKEROOT_PATH)/.config-stamp
		$(MAKE) -C $(FAKEROOT_PATH) all
		touch $@

$(FAKEROOT_PATH)/.install-stamp: $(FAKEROOT_PATH)/.build-stamp
		mkdir -p $(STAGING_DIR)/
		$(MAKE) -C $(FAKEROOT_PATH) install
		touch $@

