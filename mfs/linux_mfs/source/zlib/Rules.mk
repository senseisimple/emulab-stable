.PHONY: zlib-extract zlib-patch zlib-config zlib-build \
	zlib zlib-install zlib-clean zlib-install-target \
	zlib-install-sysroot

SYSROOT_ZLIB_SHARED = $(STAGING_DIR)/usr/lib/libz.so.$(ZLIB_VERSION)
TARGET_ZLIB_SHARED = $(TARGET_PATH)/usr/lib/libz.so.$(ZLIB_VERSION)

zlib-extract: $(ZLIB_PATH)/.extract-stamp

zlib-patch: $(ZLIB_PATH)/.patch-stamp

zlib-config: $(ZLIB_PATH)/.config-stamp

zlib-build: $(ZLIB_PATH)/libz.so.$(ZLIB_VERSION)

zlib-install-sysroot: $(SYSROOT_ZLIB_SHARED)
zlib-install-target: $(TARGET_ZLIB_SHARED)

ZLIB_CFLAGS		=	-fPIC -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64


$(ZLIB_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xjf $(SOURCE_PATH)/zlib/zlib-$(ZLIB_VERSION).tar.bz2
	touch $@

$(ZLIB_PATH)/.patch-stamp: $(ZLIB_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(ZLIB_PATH) $(SOURCE_PATH)/zlib/ '*.patch'
	touch $@

$(ZLIB_PATH)/.config-stamp: $(ZLIB_PATH)/.patch-stamp
	rm -f $(ZLIB_PATH)/config.cache
	cd $(ZLIB_PATH); \
		$(HOST_CONFIGURE_OPTS) CC=i386-linux-uclibc-gcc \
		CFLAGS="$(ZLIB_CFLAGS)" \
		./configure --prefix=/usr \
		--exec_prefix=$(STAGING_DIR)/usr/bin \
		--libdir=$(STAGING_DIR)/usr/lib \
		--includedir=$(STAGING_DIR)/usr/include \
		--shared
	touch $@

$(ZLIB_PATH)/libz.so.$(ZLIB_VERSION): $(ZLIB_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(ZLIB_PATH) all libz.a
	touch $@

$(STAGING_DIR)/usr/lib/libz.so.$(ZLIB_VERSION): $(ZLIB_PATH)/libz.so.$(ZLIB_VERSION)
	cp -dpf $(ZLIB_PATH)/libz.a $(STAGING_DIR)/usr/lib/
	cp -dpf $(ZLIB_PATH)/zlib.h $(STAGING_DIR)/usr/include/
	cp -dpf $(ZLIB_PATH)/zconf.h $(STAGING_DIR)/usr/include/
	cp -dpf $(ZLIB_PATH)/libz.so* $(STAGING_DIR)/usr/lib/
	ln -sf libz.so.$(ZLIB_VERSION) $(STAGING_DIR)/usr/lib/libz.so.1
	chmod a-x $(STAGING_DIR)/usr/lib/libz.so.$(ZLIB_VERSION)
	touch -c $@

$(TARGET_PATH)/usr/lib/libz.so.$(ZLIB_VERSION): $(STAGING_DIR)/usr/lib/libz.so.$(ZLIB_VERSION)
	mkdir -p $(TARGET_PATH)/usr/lib
	cp -dpf $(STAGING_DIR)/usr/lib/libz.so* $(TARGET_PATH)/usr/lib
	$(STRIPCMD) -s $(TARGET_PATH)/usr/lib/libz.so*
	touch -c $@

zlib-clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(ZLIB_PATH) clean
	rm -f $(ZLIB_PATH)/.build-stamp $(ZLIB_PATH)/.config-stamp

zlib: zlib-build
