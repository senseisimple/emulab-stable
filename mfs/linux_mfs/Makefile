SOURCE_PATH		=	$(PWD)/source
SCRIPTS_PATH		=	$(PWD)/scripts
TOOLCHAIN_BUILD_PATH	=	$(PWD)/toolchain_build

TARGET_BUILD_PATH	=	$(PWD)/build
TARGET_PATH		=	$(PWD)/target
TEMPLATE_PATH		=	$(PWD)/target_template
TARGET_INITRAMFS	=	$(PWD)/target.cpio.gz
FAKEROOT_ENVIRONMENT	=	$(PWD)/$(BUILD)_fs_fakeroot.env

TARGET_CC		=	i386-linux-uclibc-gcc
TARGET_CFLAGS		=	-Os -mtune=i386 -march=i386
TARGET_MODULES		=	uclibc-install-target zlib-install-target busybox-install dropbear-install linux-modules-install openssl-install-target kexec-install tmcc-install imagezip-install frisbee-install e2fsprogs-install
TARGET_FS_SIZE		=	13568

BUILDROOT_PATH		=	$(PWD)/buildroot
BUILDROOT_VERSION	=	20080705
BINUTILS_VERSION	=	2.18
GCC_VERSION		=	4.2.4
UCLIBC_VERSION		=	0.9.29

OPENSSL_VERSION		=	0.9.7m
OPENSSL_PATH		=	$(TARGET_BUILD_PATH)/openssl-$(OPENSSL_VERSION)

DROPBEAR_VERSION	=	0.51
DROPBEAR_PATH		=	$(TARGET_BUILD_PATH)/dropbear-$(DROPBEAR_VERSION)
E2FSPROGS_VERSION	=	1.41.1
E2FSPROGS_PATH		=	$(TARGET_BUILD_PATH)/e2fsprogs-$(E2FSPROGS_VERSION)
BUSYBOX_VERSION		=	1.11.0
BUSYBOX_PATH		=	$(TARGET_BUILD_PATH)/busybox-$(BUSYBOX_VERSION)
ZLIB_VERSION		=	1.2.3
ZLIB_PATH		=	$(TARGET_BUILD_PATH)/zlib-$(ZLIB_VERSION)
FAKEROOT_VERSION	=	1.8.1
FAKEROOT_PATH		=	$(TOOLCHAIN_BUILD_PATH)/fakeroot-$(FAKEROOT_VERSION)
GENEXT2FS_VERSION	=	1.4.1
GENEXT2FS_PATH 		=	$(TOOLCHAIN_BUILD_PATH)/genext2fs-$(GENEXT2FS_VERSION)
LINUX_VERSION		=	2.6.25.9
LINUX_PATH		=	$(BUILDROOT_PATH)/toolchain_build_i386/linux-$(LINUX_VERSION)
KEXEC_VERSION		=	20080324
KEXEC_PATH		=	$(TARGET_BUILD_PATH)/kexec-tools-$(KEXEC_VERSION)

STAGING_DIR		=	$(BUILDROOT_PATH)/build_i386/staging_dir/

#HOSTMAKE=make
#HOSTAR=ar
#HOSTAS=as
#HOSTCC=gcc
#HOSTCXX=g++
#HOSTLD=ld
#HOST_CFLAGS=-g -O2

#TOOLCHAIN_PATH="$(STAGING_DIR)/bin:$(STAGING_DIR)/usr/bin:$(PATH)"

CROSS_COMPILER_PREFIX=i386-linux-uclibc-
STRIPCMD=$(STAGING_DIR)/usr/bin/$(CROSS_COMPILER_PREFIX)strip
#CC=$(STAGING_DIR)/usr/bin/i386-linux-uclibc-gcc -Os  -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/include --sysroot=$(STAGING_DIR)/ -isysroot $(STAGING_DIR) -mtune=i386 -march=i386

# Hack for building uClibc -- it can't handle parallel make processes.
#MAKE1:=$(HOSTMAKE) MAKE="$(firstword $(HOSTMAKE)) -j1"

HOSTCC=gcc

HOST_CONFIGURE_OPTS=PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		AR_FOR_BUILD="$(HOSTAR)" \
		AS_FOR_BUILD="$(HOSTAS)" \
		CC_FOR_BUILD="$(HOSTCC)" \
		GCC_FOR_BUILD="$(HOSTCC)" \
		CXX_FOR_BUILD="$(HOSTCXX)" \
		LD_FOR_BUILD="$(HOSTLD)" \
		CFLAGS_FOR_BUILD="$(HOST_CFLAGS)" \
		CXXFLAGS_FOR_BUILD="$(HOST_CXXFLAGS)" \
		LDFLAGS_FOR_BUILD="$(HOST_LDFLAGS)" \
		AR_FOR_TARGET=$(CROSS_COMPILER_PREFIX)ar \
		AS_FOR_TARGET=$(CROSS_COMPILER_PREFIX)as \
		LD_FOR_TARGET=$(CROSS_COMPILER_PREFIX)ld \
		NM_FOR_TARGET=$(CROSS_COMPILER_PREFIX)nm \
		RANLIB_FOR_TARGET=$(CROSS_COMPILER_PREFIX)ranlib \
		STRIP_FOR_TARGET=$(CROSS_COMPILER_PREFIX)strip \
		OBJCOPY_FOR_TARGET=$(CROSS_COMPILER_PREFIX)objcopy

modules := $(addprefix $(SOURCE_PATH)/, buildroot busybox zlib dropbear linux fakeroot openssl kexec-tools testbed e2fsprogs)

build-env: toolchain build-env-libraries build-tools

build-env-libraries: zlib-install-sysroot openssl-install-sysroot

include $(addsuffix /Rules.mk,$(modules))

.PHONY: all root clean root-template-install toolchain build-tools build-env devices \
	root-base root-install

all: toolchain

toolchain: buildroot-build

$(TARGET_PATH)/lib/libc.so.0: toolchain
	mkdir -p $(TARGET_PATH)/lib
	cp -dpR $(BUILDROOT_PATH)/project_build_i386/uclibc/root/lib/* $(TARGET_PATH)/lib

uclibc-install-target: $(TARGET_PATH)/lib/libc.so.0

build-tools: fakeroot-install

$(FAKEROOT_ENVIRONMENT):
	touch $@

#target-base:
#	$(SCRIPTS_PATH)/mk_basic_fs.sh $(TARGET_PATH)
#	#rm -f $(FAKEROOT_ENVIRONMENT)

target-install: target-template-install $(TARGET_MODULES)

devices: $(FAKEROOT_ENVIRONMENT)
	rm -rf $(TARGET_PATH)/dev
	$(STAGING_DIR)/usr/bin/fakeroot -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/makedevs.sh \
		$(SCRIPTS_PATH)/devices $(TARGET_PATH)

permissions: $(FAKEROOT_ENVIRONMENT) devices target-install
	$(STAGING_DIR)/usr/bin/fakeroot -s $(FAKEROOT_ENVIRONMENT) \
		-i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/fixperms.sh $(TARGET_PATH)

target-initramfs: 
	$(STAGING_DIR)/usr/bin/fakeroot -i $(FAKEROOT_ENVIRONMENT) \
		$(SCRIPTS_PATH)/gen_initramfs.sh $(TARGET_PATH) $(TARGET_INITRAMFS)

target-template-install:
	mkdir -p $(TARGET_PATH)
	cp -dpR $(TEMPLATE_PATH)/* $(TARGET_PATH)

target: target-install devices permissions

build-env-clean:
	rm -rf $(BUILDROOT_PATH) $(BUILD_TOOLS)

clean:
	rm -rf $(TARGET_BUILD_PATH) $(TARGET_PATH) \
		$(FAKEROOT_ENVIRONMENT) $(TARGET_FS)
