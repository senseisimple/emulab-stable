include ../../variables.mk

E2FSPROGS_VERSION=1.41.1
E2FSPROGS_PATH=$(TARGET_BUILD_PATH)/e2fsprogs-$(E2FSPROGS_VERSION)

.PHONY: extract patch config \
	e2fsprogs install clean all

all: e2fsprogs

extract: $(E2FSPROGS_PATH)/.extract-stamp

patch: $(E2FSPROGS_PATH)/.patch-stamp

config: $(E2FSPROGS_PATH)/.config-stamp

e2fsprogs: $(E2FSPROGS_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(MAKE) -C $(E2FSPROGS_PATH)

clean:
	rm -rf $(E2FSPROGS_PATH)

LIBS	:= $(addprefix $(TARGET_PATH)/lib/, libext2fs.so libe2p.so libcom_err.so libblkid.so libuuid.so)

install: $(TARGET_PATH)/sbin/mke2fs $(TARGET_PATH)/sbin/tune2fs $(TARGET_PATH)/sbin/e2fsck $(TARGET_PATH)/usr/bin/uuidgen $(LIBS)

$(E2FSPROGS_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/e2fsprogs/e2fsprogs-$(E2FSPROGS_VERSION).tar.gz
	touch $@

$(E2FSPROGS_PATH)/.patch-stamp: $(E2FSPROGS_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(E2FSPROGS_PATH) $(SOURCE_PATH)/e2fsprogs/ '*.patch'
	touch $@

		#--build=$(GNU_HOST_NAME) \

$(E2FSPROGS_PATH)/.config-stamp: $(E2FSPROGS_PATH)/.patch-stamp
	rm -f $(E2FSPROGS_PATH)/config.cache
	(cd $(E2FSPROGS_PATH);  \
		LDFLAGS=-Wl,-rpath,$(E2FSPROGS_PATH)/lib \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./configure \
		--target=i386-linux-uclibc \
		--host=i386-linux-uclibc \
		--with-cc=$(CROSS_COMPILER_PREFIX)gcc \
		--with-linker=$(CROSS_COMPILER_PREFIX)ld \
		--prefix=/usr \
		--exec-prefix=/usr \
		--bindir=/bin \
		--sbindir=/sbin \
		--libdir=/lib \
		--libexecdir=/usr/lib \
		--sysconfdir=/etc \
		--datadir=/usr/share \
		--localstatedir=/var \
		--mandir=/usr/share/man \
		--infodir=/usr/share/info \
		--disable-debugfs --disable-imager \
		--disable-tls \
		--disable-resizer --enable-fsck \
		--disable-e2initrd-helper \
		--enable-elf-shlibs \
		--without-catgets \
	)
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(E2FSPROGS_PATH)/misc/mke2fs: $(E2FSPROGS_PATH)/.config-stamp
	LDFLAGS="-rpath ../lib" PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc
	touch $@

$(E2FSPROGS_PATH)/e2fsck/e2fsck: $(E2FSPROGS_PATH)/.config-stamp
	LDFLAGS="-rpath ../lib" PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc
	touch $@

$(E2FSPROGS_PATH)/misc/tune2fs: $(E2FSPROGS_PATH)/.config-stamp
	LDFLAGS="-rpath ../lib" PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc
	touch $@

$(E2FSPROGS_PATH)/misc/uuidgen: $(E2FSPROGS_PATH)/.config-stamp
	LDFLAGS="-rpath ../lib" PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc
	touch $@

$(E2FSPROGS_PATH)/lib/%.so: $(E2FSPROGS_PATH)/.config-stamp
	LDFLAGS="-rpath ../lib" PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/lib/%.so: $(E2FSPROGS_PATH)/lib/%.so
	install -d -m 755 $(TARGET_PATH)/lib
	install -m 644 $< $@
	$(STRIPCMD) --strip-unneeded $@
	$(STAGING_DIR)/usr/bin/ldconfig -r $(TARGET_PATH)
	touch -c $@

$(TARGET_PATH)/sbin/mke2fs: $(E2FSPROGS_PATH)/misc/mke2fs
	install -d -m 755 $(TARGET_PATH)/sbin
	install -m 755 $(E2FSPROGS_PATH)/misc/mke2fs \
		$(TARGET_PATH)/sbin/mke2fs
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/mke2fs
	touch -c $@

$(TARGET_PATH)/sbin/e2fsck: $(E2FSPROGS_PATH)/e2fsck/e2fsck
	install -d -m 755 $(TARGET_PATH)/sbin
	install -m 755 $< \
		$(TARGET_PATH)/sbin/e2fsck
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/e2fsck
	ln -sf $@ $(dir $@)/fsck.ext2
	ln -sf $@ $(dir $@)/fsck.ext3
	touch -c $@

$(TARGET_PATH)/sbin/tune2fs: $(E2FSPROGS_PATH)/misc/tune2fs
	install -d -m 755 $(TARGET_PATH)/sbin
	install -m 755 $(E2FSPROGS_PATH)/misc/tune2fs \
		$(TARGET_PATH)/sbin/tune2fs
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/tune2fs
	touch -c $@

$(TARGET_PATH)/usr/bin/uuidgen: $(E2FSPROGS_PATH)/misc/uuidgen
	install -d -m 755 $(TARGET_PATH)/usr/bin
	install -m 755 $(E2FSPROGS_PATH)/misc/uuidgen \
		$(TARGET_PATH)/usr/bin/uuidgen
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/usr/bin/uuidgen
	touch -c $@
