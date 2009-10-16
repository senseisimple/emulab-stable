MFS_ARCH	=	i386
SOURCE_PATH	=	$(PWD)/source
TARGET_PATH	=	$(PWD)/target
TARGET_BUILD_PATH	=	$(PWD)/build
SCRIPTS_PATH	=	$(PWD)/scripts
INITRAMFS_PATH	=	$(PWD)/initramfs.tmp
BUILDROOT_PATH	=	$(PWD)/buildroot
FAKEROOT_ENVIRONMENT	=	$(PWD)/fs_fakeroot.env
STAGING_DIR	=	$(PWD)/buildroot/build_$(MFS_ARCH)/staging_dir
BOOT_PATH	=	$(PWD)/boot
INITRAMFS	=	$(BOOT_PATH)/initramfs.gz

#FAKEROOT_BIN	=	$(STAGING_DIR)/usr/bin/fakeroot
FAKEROOT_BIN	=	fakeroot

MODULES := busybox zlib linux dropbear testbed hdparm target_template sudo e2fsprogs openssl wget perl portmap file
#MODULES := busybox zlib linux dropbear testbed hdparm target_template sudo e2fsprogs openssl wget portmap file
INSTALL_MODULES := $(addsuffix -install,$(MODULES))
EXTRACT_MODULES := $(addsuffix -extract,$(MODULES))
PATCH_MODULES := $(addsuffix -patch,$(MODULES))
CLEAN_MODULES := $(addsuffix -clean,$(MODULES))

.PHONY: all clean install root-template-install devices \
	root-base root-install $(MODULES) $(INSTALL_MODULES) \
	$(CLEAN_MODULES) initramfs

all: $(MODULES)

install: uclibc-install $(INSTALL_MODULES)
	cp $(SCRIPTS_PATH)/remove_unneeded_modules.pl $(BOOT_PATH)

clean:
	rm -rf $(TARGET_PATH)
	rm -f $(FAKEROOT_ENVIRONMENT)
	rm -rf $(INITRAMFS_PATH)
	rm -f $(INITRAMFS)
	rm -rf $(TARGET_BUILD_PATH)
	rm -rf $(BOOT_PATH)

$(MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$@ all

$(CLEAN_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -clean,,$@) clean

$(EXTRACT_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -extract,,$@) extract

$(PATCH_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -patch,,$@) patch

$(INSTALL_MODULES):
	$(MAKE) -C $(SOURCE_PATH)/$(subst -install,,$@) install

$(TARGET_PATH)/lib/libc.so.0:
	mkdir -p $(TARGET_PATH)/lib
	cp -dpR $(BUILDROOT_PATH)/project_build_$(MFS_ARCH)/uclibc/root/lib/* $(TARGET_PATH)/lib

uclibc-install: $(TARGET_PATH)/lib/libc.so.0

$(FAKEROOT_ENVIRONMENT):
	touch $@

devices: $(FAKEROOT_ENVIRONMENT)
	rm -rf $(TARGET_PATH)/dev
	$(FAKEROOT_BIN) -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/makedevs.sh \
		$(SCRIPTS_PATH)/devices $(TARGET_PATH)

permissions: $(FAKEROOT_ENVIRONMENT) devices install
	$(FAKEROOT_BIN) -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/fixperms.sh $(TARGET_PATH)

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
	$(FAKEROOT_BIN) -i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/gen_initramfs.sh $(INITRAMFS_PATH) $(INITRAMFS)
	rm -f $(FAKEROOT_ENVIRONMENT)
	rm -rf $(INITRAMFS_PATH)
