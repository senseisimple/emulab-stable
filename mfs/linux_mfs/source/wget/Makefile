include ../../variables.mk

WGET_VERSION	=	1.11.4
WGET_PATH	=	$(TARGET_BUILD_PATH)/wget-$(WGET_VERSION)

.PHONY: extract patch config \
	wget install clean all

all: wget

extract: $(WGET_PATH)/.extract-stamp

patch: $(WGET_PATH)/.patch-stamp

config: $(WGET_PATH)/.config-stamp

wget: $(WGET_PATH)/wget

install: $(TARGET_PATH)/usr/bin/wget

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(WGET_PATH) clean
	rm -f $(WGET_PATH)/.build-stamp $(WGET_PATH)/.config-stamp

$(WGET_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/wget/wget-$(WGET_VERSION).tar.gz
	touch $@

$(WGET_PATH)/.patch-stamp: $(WGET_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(WGET_PATH) $(SOURCE_PATH)/wget/ '*.patch'
	touch $@

$(WGET_PATH)/.config-stamp: $(WGET_PATH)/.patch-stamp
	rm -f $(WGET_PATH)/config.cache
	(cd $(WGET_PATH);  \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./configure \
		--target=x86_64-linux-uclibc \
		--host=x86_64-linux-uclibc \
		--build=x86_64-linux-gnu \
		--prefix=/usr \
		--exec-prefix=/usr \
		--bindir=/usr/bin \
		--sbindir=/usr/sbin \
		--libdir=/lib \
		--libexecdir=/usr/lib \
		--sysconfdir=/etc \
		--datadir=/usr/share \
		--localstatedir=/var \
		--mandir=/usr/man \
		--infodir=/usr/info \
	)
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(WGET_PATH)/src/wget: $(WGET_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
	        $(MAKE) -C $(WGET_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=x86_64-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/usr/bin/wget: $(WGET_PATH)/src/wget
	install -d -m 755 $(TARGET_PATH)/usr
	install -d -m 755 $(TARGET_PATH)/usr/bin
	install -m 755 $< $@
	$(STRIPCMD) --strip-unneeded $@
	touch -c $@

