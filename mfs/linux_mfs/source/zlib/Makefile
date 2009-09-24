include ../../variables.mk

.PHONY: extract patch config zlib \
	install clean install-target \
	install-sysroot all

ZLIB_VERSION		=	1.2.3
ZLIB_PATH		=	$(TARGET_BUILD_PATH)/zlib-$(ZLIB_VERSION)

SYSROOT_ZLIB_SHARED = $(STAGING_DIR)/usr/lib/libz.so.$(ZLIB_VERSION)
TARGET_ZLIB_SHARED = $(TARGET_PATH)/usr/lib/libz.so.$(ZLIB_VERSION)

all: zlib

zlib: $(ZLIB_PATH)/libz.so.$(ZLIB_VERSION)

extract: $(ZLIB_PATH)/.extract-stamp

patch: $(ZLIB_PATH)/.patch-stamp

config: $(ZLIB_PATH)/.config-stamp

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(ZLIB_PATH) clean
	rm -f $(ZLIB_PATH)/.build-stamp $(ZLIB_PATH)/.config-stamp

install: install-sysroot install-target

install-sysroot: $(SYSROOT_ZLIB_SHARED)
install-target: $(TARGET_ZLIB_SHARED)

ZLIB_CFLAGS		=	-fPIC -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

$(ZLIB_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xjf $(TARBALL_PATH)/zlib-$(ZLIB_VERSION).tar.bz2
	touch $@

$(ZLIB_PATH)/.patch-stamp: $(ZLIB_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(ZLIB_PATH) $(SOURCE_PATH)/zlib '*.patch'
	touch $@

$(ZLIB_PATH)/.config-stamp: $(ZLIB_PATH)/.patch-stamp
	rm -f $(ZLIB_PATH)/config.cache
	cd $(ZLIB_PATH); \
		$(HOST_CONFIGURE_OPTS) CC=x86_64-linux-uclibc-gcc \
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
