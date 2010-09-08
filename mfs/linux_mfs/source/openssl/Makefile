include ../../variables.mk

.PHONY: extract patch config \
	openssl install clean all

OPENSSL_VERSION =	0.9.8n
OPENSSL_PATH	=	$(TARGET_BUILD_PATH)/openssl-$(OPENSSL_VERSION)

SYSROOT_OPENSSL_SHARED = $(STAGING_DIR)/usr/lib/libcrypto.a
TARGET_OPENSSL_SHARED = $(TARGET_PATH)/usr/lib/libcrypto.so.0.9.8

all: openssl

extract: $(OPENSSL_PATH)/.extract-stamp

patch: $(OPENSSL_PATH)/.patch-stamp

config: $(OPENSSL_PATH)/.config-stamp

openssl: $(OPENSSL_PATH)/apps/openssl

clean:
	rm -rf $(OPENSSL_PATH)

install: $(SYSROOT_OPENSSL_SHARED) $(TARGET_OPENSSL_SHARED)

OPENSSL_CFLAGS		=	-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

$(OPENSSL_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/openssl-$(OPENSSL_VERSION).tar.gz
	touch $@

$(OPENSSL_PATH)/.patch-stamp: $(OPENSSL_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(OPENSSL_PATH) $(SOURCE_PATH)/openssl/ '*.patch'
	# sigh... we have to resort to this just to set a gcc flag.
	# grumble.. and of course make sure to escape any '/' in CFLAGS
	sed '/CFLAG=/s,/;, $(shell echo '$(TARGET_CFLAGS)' | sed -e 's/\//\\\\\//g')/;,' \
		$(OPENSSL_PATH)/Configure > $(OPENSSL_PATH)/Configure.mod
	mv -f $(OPENSSL_PATH)/Configure.mod $(OPENSSL_PATH)/Configure
	chmod a+x $(OPENSSL_PATH)/Configure
	touch $@

$(OPENSSL_PATH)/.config-stamp: $(OPENSSL_PATH)/.patch-stamp
	(cd $(OPENSSL_PATH); \
		CFLAGS="-DOPENSSL_NO_KRB5 -DOPENSSL_NO_IDEA -DOPENSSL_NO_MDC2 -DOPENSSL_NO_RC5 $(TARGET_CFLAGS)" \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		MACHINE=$(MFS_ARCH) \
		./config --prefix=/usr \
			--openssldir=/usr/lib/ssl -L$(STAGING_DIR)/lib -ldl \
			-I$(STAGING_DIR)/usr/include $(OPENSSL_OPTS) threads \
			shared no-idea no-mdc2 no-rc5)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) CC=$(MFS_ARCH)-linux-uclibc-gcc -C $(OPENSSL_PATH) depend
	touch $@

$(OPENSSL_PATH)/apps/openssl: $(OPENSSL_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) CC=$(MFS_ARCH)-linux-uclibc-gcc -C $(OPENSSL_PATH) all build-shared
	# Work around openssl build bug to link libssl.so with libcrypto.so.
	-rm $(OPENSSL_PATH)/libssl.so.*.*.*
	$(MAKE) PATH=$(PATH):$(STAGING_DIR)/usr/bin \
		CC=$(MFS_ARCH)-linux-uclibc-gcc -C $(OPENSSL_PATH) do_linux-shared
	touch $@

$(STAGING_DIR)/usr/lib/libcrypto.a: $(OPENSSL_PATH)/apps/openssl
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) \
		CC=$(TARGET_CC) INSTALL_PREFIX=$(STAGING_DIR) \
		-C $(OPENSSL_PATH) install
	cp -fa $(OPENSSL_PATH)/libcrypto.so* $(STAGING_DIR)/usr/lib/
	chmod a-x $(STAGING_DIR)/usr/lib/libcrypto.so.0.9.8
	(cd $(STAGING_DIR)/usr/lib; \
	 ln -fs libcrypto.so.0.9.8 libcrypto.so; \
	 ln -fs libcrypto.so.0.9.8 libcrypto.so.0; \
	)
	cp -fa $(OPENSSL_PATH)/libssl.so* $(STAGING_DIR)/usr/lib/
	chmod a-x $(STAGING_DIR)/usr/lib/libssl.so.0.9.8
	(cd $(STAGING_DIR)/usr/lib; \
	 ln -fs libssl.so.0.9.8 libssl.so; \
	 ln -fs libssl.so.0.9.8 libssl.so.0; \
	)
	touch -c $@

$(TARGET_PATH)/usr/lib/libcrypto.so.0.9.8: $(STAGING_DIR)/usr/lib/libcrypto.a
	mkdir -p $(TARGET_PATH)/usr/lib
	mkdir -p $(TARGET_PATH)/usr/lib/ssl
	mkdir -p $(TARGET_PATH)/usr/lib/engines
	cp -fa $(STAGING_DIR)/usr/lib/libcrypto.so* $(TARGET_PATH)/usr/lib/
	cp -fa $(STAGING_DIR)/usr/lib/libssl.so* $(TARGET_PATH)/usr/lib/
	#cp -fa $(STAGING_DIR)/bin/openssl $(TARGET_PATH)/bin/
	$(STRIPCMD) $(TARGET_PATH)/usr/lib/libssl.so.0.9.8
	$(STRIPCMD) $(TARGET_PATH)/usr/lib/libcrypto.so.0.9.8

$(TARGET_DIR)/usr/lib/libssl.a: $(STAGING_DIR)/usr/lib/libcrypto.a
	mkdir -p $(TARGET_DIR)/usr/include
	cp -a $(STAGING_DIR)/usr/include/openssl $(TARGET_DIR)/usr/include/
	cp -dpf $(STAGING_DIR)/usr/lib/libssl.a $(TARGET_DIR)/usr/lib/
	cp -dpf $(STAGING_DIR)/usr/lib/libcrypto.a $(TARGET_DIR)/usr/lib/
	touch -c $@
