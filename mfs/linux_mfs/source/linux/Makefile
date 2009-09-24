include ../../variables.mk

LINUX_VERSION		=	2.6.27.8
LINUX_PATH		=	$(TARGET_BUILD_PATH)/linux-$(LINUX_VERSION)

KERNEL_CONFIG	=	$(SOURCE_PATH)/linux/linux.config
TARGET_MODULE_PATH =	$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)

export ARCH=x86_64
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
		cp $(LINUX_PATH)/arch/x86_64/boot/bzImage $(TOPDIR)/boot/vmlinuz-$(LINUX_VERSION)
		cp $(LINUX_PATH)/System.map $(TOPDIR)/boot/System.map-$(LINUX_VERSION)
		cp $(LINUX_PATH)/.config $(TOPDIR)/boot/config-$(LINUX_VERSION)

modules-install: modules
		mkdir -p $(TARGET_PATH)/lib/modules
		$(MAKE) -C $(LINUX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) modules_install
		/sbin/depmod -b $(TARGET_PATH) $(LINUX_VERSION)
		grep -v 'ide[-_]pci[-_]generic' $(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias > \
			$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias.mod
		mv -f $(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias.mod \
			$(TARGET_PATH)/lib/modules/$(LINUX_VERSION)/modules.alias

$(LINUX_PATH)/.extract-stamp:
		mkdir -p $(TARGET_BUILD_PATH)
		cd $(TARGET_BUILD_PATH); tar xjf $(SOURCE_PATH)/linux/linux-$(LINUX_VERSION).tar.bz2
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

