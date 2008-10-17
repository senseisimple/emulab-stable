.PHONY: e2fsprogs-extract e2fsprogs-patch e2fsprogs-config e2fsprogs-build \
	e2fsprogs e2fsprogs-install e2fsprogs-clean

e2fsprogs-extract: $(E2FSPROGS_PATH)/.extract-stamp

e2fsprogs-patch: $(E2FSPROGS_PATH)/.patch-stamp

e2fsprogs-config: $(E2FSPROGS_PATH)/.config-stamp

e2fsprogs-build: $(E2FSPROGS_PATH)/misc/mke2fs

e2fsprogs-install: $(TARGET_PATH)/sbin/mke2fs

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
		--without-catgets \
	)
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(E2FSPROGS_PATH)/misc/mke2fs: $(E2FSPROGS_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc \
		PROGRAMS="e2fsprogs dbclient e2fsprogskey e2fsprogsconvert scp" \
		MULTI=1 SCPPROGRESS=1
	touch $@

$(TARGET_PATH)/sbin/mke2fs: $(E2FSPROGS_PATH)/misc/mke2fs
	install -d -m 755 $(TARGET_PATH)/sbin
	install -m 755 $(E2FSPROGS_PATH)/misc/mke2fs \
		$(TARGET_PATH)/sbin/mke2fs
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/sbin/mke2fs
	touch -c $@

e2fsprogs-clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(E2FSPROGS_PATH) clean
	rm -f $(E2FSPROGS_PATH)/.build-stamp $(E2FSPROGS_PATH)/.config-stamp

e2fsprogs: e2fsprogs-build
