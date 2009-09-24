include ../../variables.mk

SUDO_VERSION	=	1.6.9p17
SUDO_PATH	=	$(TARGET_BUILD_PATH)/sudo-$(SUDO_VERSION)

.PHONY: extract patch config \
	sudo install clean all

all: sudo

extract: $(SUDO_PATH)/.extract-stamp

patch: $(SUDO_PATH)/.patch-stamp

config: $(SUDO_PATH)/.config-stamp

sudo: $(SUDO_PATH)/sudo

install: $(TARGET_PATH)/usr/bin/sudo

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(SUDO_PATH) clean
	rm -f $(SUDO_PATH)/.build-stamp $(SUDO_PATH)/.config-stamp

$(SUDO_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(SOURCE_PATH)/sudo/sudo-$(SUDO_VERSION).tar.gz
	touch $@

$(SUDO_PATH)/.patch-stamp: $(SUDO_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(SUDO_PATH) $(SOURCE_PATH)/sudo/ '*.patch'
	touch $@

$(SUDO_PATH)/.config-stamp: $(SUDO_PATH)/.patch-stamp
	rm -f $(SUDO_PATH)/config.cache
	(cd $(SUDO_PATH);  \
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
		--without-lecture \
		--without-sendmail \
		--without-umask \
		--with-logging=syslog \
		--without-interfaces \
		--without-pam \
		$(SUDO_EXTRA_CONFIG) \
	)
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(SUDO_PATH)/sudo: $(SUDO_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
	        $(MAKE) -C $(SUDO_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=x86_64-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/usr/bin/sudo: $(SUDO_PATH)/sudo
	install -d -m 755 $(TARGET_PATH)/usr
	install -d -m 755 $(TARGET_PATH)/usr/bin
	install -m 755 $(SUDO_PATH)/sudo \
		$(TARGET_PATH)/usr/bin/sudo
	$(STRIPCMD) --strip-unneeded $(TARGET_PATH)/usr/bin/sudo
	mkdir -p $(TARGET_PATH)/usr/local/bin
	ln -sf /usr/bin/sudo $(TARGET_PATH)/usr/local/bin
	touch -c $@

