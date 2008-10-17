.PHONY: buildroot-extract buildroot-patch buildroot-config buildroot-sources buildroot-build \
	buildroot buildroot-install buildroot-clean

buildroot-extract: $(BUILDROOT_PATH)/.extract-stamp

buildroot-patch: $(BUILDROOT_PATH)/.patch-stamp

buildroot-config: $(BUILDROOT_PATH)/.config-stamp

buildroot-sources: $(BUILDROOT_PATH)/.sources-stamp

buildroot-build: $(BUILDROOT_PATH)/.build-stamp

buildroot-install: $(BUILDROOT_PATH)/.install-stamp

buildroot-clean:
		$(MAKE) -C $(BUILDROOT_PATH) clean
		rm -f $(BUILDROOT_PATH)/.build-stamp

buildroot: buildroot-build

$(BUILDROOT_PATH)/.extract-stamp:
		tar xjf $(SOURCE_PATH)/buildroot/buildroot-$(BUILDROOT_VERSION).tar.bz2
		touch $@

$(BUILDROOT_PATH)/.patch-stamp: $(BUILDROOT_PATH)/.extract-stamp
		#$(SCRIPTS_PATH)/patch-kernel.sh $(BUILDROOT_PATH) $(SOURCE_PATH)/buildroot/ '*.patch'
		touch $@

$(BUILDROOT_PATH)/.config-stamp: $(BUILDROOT_PATH)/.patch-stamp
		cp $(SOURCE_PATH)/buildroot/buildroot.config $(BUILDROOT_PATH)/.config
		cd $(BUILDROOT_PATH); unset CC; make oldconfig
		touch $@

$(BUILDROOT_PATH)/.sources-stamp: $(BUILDROOT_PATH)/.config-stamp
		mkdir -p $(BUILDROOT_PATH)/dl
		cp $(SOURCE_PATH)/toolchain/* $(BUILDROOT_PATH)/dl
		cp $(SOURCE_PATH)/uClibc/uClibc-$(UCLIBC_VERSION).tar.bz2 $(BUILDROOT_PATH)/dl
		cp $(SOURCE_PATH)/linux/linux-$(LINUX_VERSION).tar.bz2 $(BUILDROOT_PATH)/dl
		touch $@

$(BUILDROOT_PATH)/.build-stamp: $(BUILDROOT_PATH)/.sources-stamp
		unset CC; $(MAKE) -C $(BUILDROOT_PATH) all
		touch $@

$(BUILDROOT_PATH)/.install-stamp: $(BUILDROOT_PATH)/.build-stamp
		#mkdir -p $(STAGING_DIR)/
		#$(MAKE) -C $(BUILDROOT_PATH) install
		touch $@

