include ../../variables.mk

LINUX_VERSION		=	2.6.32.9
LINUX_PATH		=	$(TARGET_BUILD_PATH)/linux-$(LINUX_VERSION)

KERNEL_CONFIG	=	$(SOURCE_PATH)/linux/linux.config.$(MFS_ARCH)
TARGET_MODULE_PATH =	$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)

MODULE_TARBALL_PATH = $(TOPDIR)/boot/modules-$(LINUX_VERSION).tar.gz

export ARCH=$(MFS_ARCH)
export INSTALL_MOD_PATH=$(TARGET_PATH)

.PHONY: extract patch config bzImage \
	modules clean menuconfig bzImage-install \
	modules-install all

all: bzImage modules

extract: $(LINUX_PATH)/.extract-stamp

patch: $(LINUX_PATH)/.patch-stamp

config: $(LINUX_PATH)/.config-stamp

headers: $(LINUX_PATH)/.headers-stamp

menuconfig:
		$(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) menuconfig

clean:
		$(MAKE) -C $(LINUX_PATH) clean

install: bzImage-install modules-install

bzImage-install: bzImage
		mkdir -p $(TOPDIR)/boot
		cp $(LINUX_PATH)/arch/$(MFS_ARCH)/boot/bzImage $(TOPDIR)/boot/vmlinuz-$(LINUX_VERSION)
		cp $(LINUX_PATH)/System.map $(TOPDIR)/boot/System.map-$(LINUX_VERSION)
		cp $(LINUX_PATH)/.config $(TOPDIR)/boot/config-$(LINUX_VERSION)

modules-install: modules
		mkdir -p $(TARGET_BUILD_PATH)/lib/modules
		$(MAKE) -C $(LINUX_PATH) INSTALL_MOD_PATH=$(TARGET_BUILD_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) modules_install
		rm -f $(TARGET_BUILD_PATH)/lib/modules/$(LINUX_VERSION)/build
		rm -f $(TARGET_BUILD_PATH)/lib/modules/$(LINUX_VERSION)/source
		/sbin/depmod -b $(TARGET_BUILD_PATH) $(LINUX_VERSION)
		cp -dpR $(TARGET_BUILD_PATH)/lib $(TARGET_PATH)
		(cd $(TARGET_BUILD_PATH) && tar cvzf $(MODULE_TARBALL_PATH) ./lib)
		rm -rf $(TARGET_BUILD_PATH)/lib
		#grep -v 'ide[-_]pci[-_]generic' $(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias > \
		#	$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias.mod
		#mv -f $(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias.mod \
		#	$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias

$(LINUX_PATH)/.extract-stamp:
		mkdir -p $(TARGET_BUILD_PATH)
		cd $(TARGET_BUILD_PATH); tar xjf $(TARBALL_PATH)/linux-$(LINUX_VERSION).tar.bz2
		touch $@

$(LINUX_PATH)/.patch-stamp: $(LINUX_PATH)/.extract-stamp
		$(SCRIPTS_PATH)/patch-kernel.sh $(LINUX_PATH) $(SOURCE_PATH)/linux/ '*.patch'
		touch $@

$(LINUX_PATH)/.config-stamp: $(LINUX_PATH)/.patch-stamp
		cp $(KERNEL_CONFIG) $(LINUX_PATH)/.config
		$(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) oldconfig
		touch $@

bzImage: $(LINUX_PATH)/.config-stamp
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) bzImage

modules: $(LINUX_PATH)/.config-stamp
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) modules

