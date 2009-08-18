include ../../variables.mk

FILE_VERSION	=	5.03
LIBMAGIC_VERSION = 	1.0.0
FILE_PATH	=	$(TARGET_BUILD_PATH)/file-$(FILE_VERSION)

.PHONY: extract patch config \
	file install clean all

all: file

extract: $(FILE_PATH)/.extract-stamp

patch: $(FILE_PATH)/.patch-stamp

config: $(FILE_PATH)/.config-stamp

file: $(FILE_PATH)/src/.libs/file

install: $(TARGET_PATH)/usr/bin/file $(TARGET_PATH)/usr/share/misc/magic

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(FILE_PATH) clean
	rm -f $(FILE_PATH)/.build-stamp $(FILE_PATH)/.config-stamp

$(FILE_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/file/file-$(FILE_VERSION).tar.gz
	touch $@

$(FILE_PATH)/.patch-stamp: $(FILE_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(FILE_PATH) $(SOURCE_PATH)/file/ '*.patch'
	touch $@

$(FILE_PATH)/.config-stamp: $(FILE_PATH)/.patch-stamp
	rm -f $(FILE_PATH)/config.cache
	(cd $(FILE_PATH);  \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./configure \
		--target=i386-linux-uclibc \
		--host=i386-linux-uclibc \
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

$(FILE_PATH)/src/.libs/file: $(FILE_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
	        $(MAKE) -C $(FILE_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=i386-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/usr/share/misc/magic:
	install -d -m 755 $(TARGET_PATH)/usr
	install -d -m 755 $(TARGET_PATH)/usr/share
	install -d -m 755 $(TARGET_PATH)/usr/share/misc
	cat $(FILE_PATH)/magic/Header $(FILE_PATH)/magic/Localstuff \
		$(FILE_PATH)/magic/Magdir/* > $@
	touch -c $@

$(TARGET_PATH)/usr/bin/file: $(FILE_PATH)/src/.libs/file
	install -d -m 755 $(TARGET_PATH)/usr
	install -d -m 755 $(TARGET_PATH)/usr/bin
	install -d -m 755 $(TARGET_PATH)/usr/lib
	install -m 755 $< $@
	install -m 644 $(FILE_PATH)/src/.libs/libmagic.so.$(LIBMAGIC_VERSION) $(TARGET_PATH)/usr/lib
	$(STRIPCMD) --strip-unneeded $@
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/usr/lib/libmagic.so.$(LIBMAGIC_VERSION)
	$(STAGING_DIR)/usr/bin/ldconfig -r $(TARGET_PATH)
	touch -c $@

