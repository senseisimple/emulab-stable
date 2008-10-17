.PHONY: kexec-extract kexec-patch kexec-config kexec-build \
	kexec kexec-install kexec-clean

kexec-extract: $(KEXEC_PATH)/.extract-stamp

kexec-patch: $(KEXEC_PATH)/.patch-stamp

kexec-config: $(KEXEC_PATH)/.config-stamp

kexec-build: $(KEXEC_PATH)/build/sbin/kexec

kexec-install: $(TARGET_PATH)/sbin/kexec

kexec-clean:
		$(MAKE) -C $(KEXEC_PATH) clean
		rm -f $(KEXEC_PATH)/.build-stamp

kexec: kexec-build

$(KEXEC_PATH)/.extract-stamp:
		mkdir -p $(TARGET_BUILD_PATH)
		cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/kexec-tools/kexec-tools_$(KEXEC_VERSION).tar.gz
		touch $@

$(KEXEC_PATH)/.patch-stamp: $(KEXEC_PATH)/.extract-stamp
		$(SCRIPTS_PATH)/patch-kernel.sh $(KEXEC_PATH) $(SOURCE_PATH)/kexec-tools/ '*.patch'
		touch $@

$(KEXEC_PATH)/.config-stamp: $(KEXEC_PATH)/.patch-stamp
	rm -f $(KEXEC_PATH)/config.cache
	cd $(KEXEC_PATH); \
		$(HOST_CONFIGURE_OPTS) \
		CFLAGS="$(KEXEC_CFLAGS)" \
		./configure --prefix=/usr \
		--exec_prefix=$(STAGING_DIR)/usr/bin \
		--host=i386-linux-uclibc \
		--libdir=$(STAGING_DIR)/usr/lib \
		--includedir=$(STAGING_DIR)/usr/include
	touch $@

$(KEXEC_PATH)/build/sbin/kexec: $(KEXEC_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(KEXEC_PATH) $(HOST_CONFIGURE_OPTS) ARCH=i386 build/sbin/kexec
	touch $@

$(TARGET_PATH)/sbin/kexec: $(KEXEC_PATH)/build/sbin/kexec
		mkdir -p $(TARGET_PATH)/sbin
		cp $< $@
		$(STRIPCMD) -s $@
		touch $@

