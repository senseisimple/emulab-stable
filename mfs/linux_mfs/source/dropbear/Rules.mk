.PHONY: dropbear-extract dropbear-patch dropbear-config dropbear-build \
	dropbear dropbear-install dropbear-clean

dropbear-extract: $(DROPBEAR_PATH)/.extract-stamp

dropbear-patch: $(DROPBEAR_PATH)/.patch-stamp

dropbear-config: $(DROPBEAR_PATH)/.config-stamp

dropbear-build: $(DROPBEAR_PATH)/dropbearmulti

dropbear-install: $(TARGET_PATH)/usr/sbin/dropbear

$(DROPBEAR_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/dropbear/dropbear-$(DROPBEAR_VERSION).tar.gz
	touch $@

$(DROPBEAR_PATH)/.patch-stamp: $(DROPBEAR_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(DROPBEAR_PATH) $(SOURCE_PATH)/dropbear/ '*.patch'
	touch $@

		#--build=$(GNU_HOST_NAME) \

$(DROPBEAR_PATH)/.config-stamp: $(DROPBEAR_PATH)/.patch-stamp
	rm -f $(DROPBEAR_PATH)/config.cache
	cd $(DROPBEAR_PATH); autoconf
	cd $(DROPBEAR_PATH);  \
		$(HOST_CONFIGURE_OPTS) \
		./configure --prefix=/usr \
		--target=i386-linux-uclibc \
		--host=i386-linux-uclibc \
		--sysconfdir=/etc \
		--localstatedir=/var \
		--libdir=$(STAGING_DIR)/usr/lib \
		--includedir=$(STAGING_DIR)/usr/include \
		--with-shared
	touch $@

$(DROPBEAR_PATH)/dropbearmulti: $(DROPBEAR_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(DROPBEAR_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc \
		PROGRAMS="dropbear dbclient dropbearkey dropbearconvert scp" \
		MULTI=1 SCPPROGRESS=1
	touch $@

$(TARGET_PATH)/usr/sbin/dropbear: $(DROPBEAR_PATH)/dropbearmulti
	install -d -m 755 $(TARGET_PATH)/usr/sbin
	install -d -m 755 $(TARGET_PATH)/usr/bin
	install -m 755 $(DROPBEAR_PATH)/dropbearmulti \
		$(TARGET_PATH)/usr/sbin/dropbear
	$(STRIPCMD) -s $(TARGET_PATH)/usr/sbin/dropbear
	ln -snf ../sbin/dropbear $(TARGET_PATH)/usr/bin/scp
	ln -snf ../sbin/dropbear $(TARGET_PATH)/usr/bin/ssh
	ln -snf ../sbin/dropbear $(TARGET_PATH)/usr/bin/dbclient
	ln -snf ../sbin/dropbear $(TARGET_PATH)/usr/bin/dropbearkey
	ln -snf ../sbin/dropbear $(TARGET_PATH)/usr/bin/dropbearconvert
	mkdir -p $(TARGET_PATH)/etc/init.d
	install -m 755 $(SOURCE_PATH)/dropbear/S50dropbear $(TARGET_PATH)/etc/init.d/S50dropbear
	mkdir -p $(TARGET_PATH)/usr/lib
	touch -c $@

dropbear-clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(DROPBEAR_PATH) clean
	rm -f $(DROPBEAR_PATH)/.build-stamp $(DROPBEAR_PATH)/.config-stamp

dropbear: dropbear-build
