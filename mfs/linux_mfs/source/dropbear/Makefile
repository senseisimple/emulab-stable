include ../../variables.mk

DROPBEAR_VERSION	=	0.52
DROPBEAR_PATH		=	$(TARGET_BUILD_PATH)/dropbear-$(DROPBEAR_VERSION)

.PHONY: extract patch config build \
	dropbear install clean all

all: dropbear

extract: $(DROPBEAR_PATH)/.extract-stamp

patch: $(DROPBEAR_PATH)/.patch-stamp

config: $(DROPBEAR_PATH)/.config-stamp

dropbear: $(DROPBEAR_PATH)/dropbearmulti

install: $(TARGET_PATH)/usr/sbin/dropbear

$(DROPBEAR_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/dropbear-$(DROPBEAR_VERSION).tar.gz
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
		--target=$(MFS_ARCH)-linux-uclibc \
		--host=$(MFS_ARCH)-linux-uclibc \
		--sysconfdir=/etc \
		--localstatedir=/var \
		--libdir=$(STAGING_DIR)/usr/lib \
		--includedir=$(STAGING_DIR)/usr/include \
		--with-shared
	touch $@

$(DROPBEAR_PATH)/dropbearmulti: $(DROPBEAR_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(DROPBEAR_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=$(MFS_ARCH)-linux-uclibc-gcc \
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
	mkdir -p $(TARGET_PATH)/etc/dropbear
	ln -snf dropbear $(TARGET_PATH)/etc/ssh
	touch -c $@

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(DROPBEAR_PATH) clean
	rm -f $(DROPBEAR_PATH)/.build-stamp $(DROPBEAR_PATH)/.config-stamp
