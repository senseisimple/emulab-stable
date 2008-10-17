KERNEL_CONFIG	=	$(SOURCE_PATH)/linux/linux.config
LINUX_HEADERS_PATH =	$(TOOLCHAIN_BUILD_PATH)/linux
TARGET_MODULE_PATH =	$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)

export ARCH=i386
export INSTALL_MOD_PATH=$(TARGET_PATH)

.PHONY: linux linux-extract linux-patch linux-config linux-bzImage \
	linux-modules linux-clean linux-menuconfig linux-bzImage-install \
	linux-modules-install linux-install linux-headers

linux-extract: $(LINUX_PATH)/.extract-stamp

linux-patch: $(LINUX_PATH)/.patch-stamp

linux-config: $(LINUX_PATH)/.config-stamp

linux-headers: $(LINUX_PATH)/.headers-stamp

linux-menuconfig:
		$(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) menuconfig

linux-clean:
		$(MAKE) -C $(LINUX_PATH) clean

linux-bzImage-install: linux-bzImage
		mkdir -p $(TARGET_PATH)/boot
		cp $(LINUX_PATH)/arch/i386/boot/bzImage $(TARGET_PATH)/boot/vmlinuz-$(LINUX_VERSION)
		cp $(LINUX_PATH)/System.map $(TARGET_PATH)/boot/System.map-$(LINUX_VERSION)
		cp $(LINUX_PATH)/.config $(TARGET_PATH)/boot/config-$(LINUX_VERSION)

linux-modules-install: linux-modules
		mkdir -p $(TARGET_PATH)/lib/modules
		$(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) modules_install
		/sbin/depmod -b $(TARGET_PATH) $(LINUX_VERSION)
		grep -v 'ide[-_]pci[-_]generic' $(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias > \
			$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias.mod
		mv -f $(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias.mod \
			$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias

$(LINUX_PATH)/.headers-stamp: $(LINUX_PATH)/.config-stamp
		$(MAKE) -C $(LINUX_PATH) \
			ARCH=i386 \
			INSTALL_HDR_PATH=$(TOOLCHAIN_BUILD_PATH)/linux \
			headers_install
		touch $@

linux-install: linux-bzImage-install linux-modules-install

linux:		linux-bzImage linux-modules

$(LINUX_PATH)/.extract-stamp:
		mkdir -p $(TOOLCHAIN_BUILD_PATH)
		cd $(TOOLCHAIN_BUILD_PATH); tar xjf $(SOURCE_PATH)/linux/linux-$(LINUX_VERSION).tar.bz2
		touch $@

$(LINUX_PATH)/.patch-stamp: $(LINUX_PATH)/.extract-stamp
		$(SCRIPTS_PATH)/patch-kernel.sh $(LINUX_PATH) $(SOURCE_PATH)/linux/ '*.patch'
		touch $@

$(LINUX_PATH)/.config-stamp: $(LINUX_PATH)/.patch-stamp
		cp $(KERNEL_CONFIG) $(LINUX_PATH)/.config
		$(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) oldconfig
		touch $@

linux-bzImage: $(LINUX_PATH)/.config-stamp
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) bzImage

linux-modules: $(LINUX_PATH)/.config-stamp
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) modules

