MFS_ARCH	=	x86_64
SOURCE_PATH	=	$(PWD)/source
TARGET_PATH	=	$(PWD)/target
TARGET_BUILD_PATH	=	$(PWD)/build
SCRIPTS_PATH	=	$(PWD)/scripts
INITRAMFS_PATH	=	$(PWD)/initramfs.tmp
BUILDROOT_VERSION	=	2010.02
BUILDROOT_PATH		=	$(PWD)/buildroot-$(BUILDROOT_VERSION)
FAKEROOT_ENVIRONMENT	=	$(PWD)/fs_fakeroot.env
STAGING_DIR	=	$(PWD)/buildroot/build_$(MFS_ARCH)/staging_dir
BOOT_PATH	=	$(PWD)/boot
INITRAMFS	=	$(BOOT_PATH)/initramfs.lzma

#FAKEROOT_BIN	=	$(STAGING_DIR)/usr/bin/fakeroot
FAKEROOT_BIN	=	fakeroot

MODULES := busybox zlib linux dropbear testbed hdparm target_template sudo \
	e2fsprogs openssl wget perl portmap file ethtool ncurses jove tcsh
#MODULES := busybox zlib linux dropbear testbed hdparm target_template sudo e2fsprogs openssl wget portmap file
INSTALL_MODULES := $(addsuffix -install,$(MODULES))
EXTRACT_MODULES := $(addsuffix -extract,$(MODULES) toolchain)
PATCH_MODULES := $(addsuffix -patch,$(MODULES) toolchain)
CONFIG_MODULES := $(addsuffix -config,$(MODULES) toolchain)
CLEAN_MODULES := $(addsuffix -clean,$(MODULES) toolchain)

.PHONY: all clean install root-template-install devices \
	root-base root-install $(MODULES) $(INSTALL_MODULES) \
	$(CLEAN_MODULES) initramfs

all: $(MODULES) initramfs

install: uclibc-install $(INSTALL_MODULES)
	cp $(SCRIPTS_PATH)/remove_unneeded_modules.pl $(BOOT_PATH)
	cp $(SCRIPTS_PATH)/compress_initramfs $(BOOT_PATH)
	cp $(SCRIPTS_PATH)/extract_initramfs $(BOOT_PATH)

clean:
	rm -rf $(TARGET_PATH)
	rm -f $(FAKEROOT_ENVIRONMENT)
	rm -rf $(INITRAMFS_PATH)
	rm -f $(INITRAMFS)
	rm -rf $(TARGET_BUILD_PATH)
	rm -rf $(BOOT_PATH)

toolchain: $(BUILDROOT_PATH)/toolchain-built

$(BUILDROOT_PATH)/toolchain-built:
	$(MAKE) -C $(SOURCE_PATH)/toolchain all
	$(MAKE) -C $(SOURCE_PATH)/zlib install-sysroot
	$(MAKE) -C $(SOURCE_PATH)/openssl install
	$(MAKE) -C $(SOURCE_PATH)/ncurses install
	touch $(BUILDROOT_PATH)/toolchain-built

$(MODULES): toolchain
	$(MAKE) -C $(SOURCE_PATH)/$@ all

$(CLEAN_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -clean,,$@) clean

$(EXTRACT_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -extract,,$@) extract

$(PATCH_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -patch,,$@) patch

$(CONFIG_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -config,,$@) config

$(INSTALL_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -install,,$@) install

$(TARGET_PATH)/lib/libc.so.0:
	mkdir -p $(TARGET_PATH)/lib
	cp -dpR $(BUILDROOT_PATH)/output/target/lib/* $(TARGET_PATH)/lib

uclibc-install: $(TARGET_PATH)/lib/libc.so.0

$(FAKEROOT_ENVIRONMENT):
	touch $@

initramfs: install
	rm -rf $(INITRAMFS_PATH)
	cp -dpR $(TARGET_PATH) $(INITRAMFS_PATH)
	cat /dev/null > $(FAKEROOT_ENVIRONMENT)
	rm -rf $(INITRAMFS_PATH)/dev
	$(FAKEROOT_BIN) -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/makedevs.sh \
		$(SCRIPTS_PATH)/devices $(INITRAMFS_PATH)
	$(FAKEROOT_BIN) -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/fixperms.sh $(INITRAMFS_PATH)
	$(FAKEROOT_BIN) -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/misc_fixup.sh $(TARGET_PATH)
	$(FAKEROOT_BIN) -i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/gen_initramfs.sh $(INITRAMFS_PATH) $(INITRAMFS)
	rm -f $(FAKEROOT_ENVIRONMENT)
	rm -rf $(INITRAMFS_PATH)
