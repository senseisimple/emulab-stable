.PHONY: genext2fs-extract genext2fs-patch genext2fs-config genext2fs-build \
	genext2fs genext2fs-install genext2fs-clean

genext2fs-extract: $(GENEXT2FS_PATH)/.extract-stamp

genext2fs-patch: $(GENEXT2FS_PATH)/.patch-stamp

genext2fs-config: $(GENEXT2FS_PATH)/.config-stamp

genext2fs-build: $(GENEXT2FS_PATH)/.build-stamp

genext2fs-install: $(GENEXT2FS_PATH)/.install-stamp

genext2fs-clean:
		$(MAKE) -C $(GENEXT2FS_PATH) clean
		rm -f $(GENEXT2FS_PATH)/.build-stamp

genext2fs: genext2fs-build

$(GENEXT2FS_PATH)/.extract-stamp:
		mkdir -p $(TOOLCHAIN_BUILD_PATH)
		cd $(TOOLCHAIN_BUILD_PATH); tar xzf $(SOURCE_PATH)/genext2fs/genext2fs-$(GENEXT2FS_VERSION).tar.gz
		touch $@

$(GENEXT2FS_PATH)/.patch-stamp: $(GENEXT2FS_PATH)/.extract-stamp
		$(SCRIPTS_PATH)/patch-kernel.sh $(GENEXT2FS_PATH) $(SOURCE_PATH)/genext2fs/ '*.patch'
		touch $@

$(GENEXT2FS_PATH)/.config-stamp: $(GENEXT2FS_PATH)/.patch-stamp
		cd $(GENEXT2FS_PATH); ./configure \
			--prefix=$(STAGING_DIR)/usr
		touch $@

$(GENEXT2FS_PATH)/.build-stamp: $(GENEXT2FS_PATH)/.config-stamp
		$(MAKE) -C $(GENEXT2FS_PATH) all
		touch $@

$(GENEXT2FS_PATH)/.install-stamp: $(GENEXT2FS_PATH)/.build-stamp
		mkdir -p $(STAGING_DIR)/
		$(MAKE) -C $(GENEXT2FS_PATH) install
		touch $@

