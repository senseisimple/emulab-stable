include ../../variables.mk

TCSH_VERSION	=	6.14.00
TCSH_PATH	=	$(TARGET_BUILD_PATH)/tcsh-$(TCSH_VERSION)

.PHONY: extract patch config \
	tcsh install clean all

all: tcsh

extract: $(TCSH_PATH)/.extract-stamp

patch: $(TCSH_PATH)/.patch-stamp

config: $(TCSH_PATH)/.config-stamp

tcsh: $(TCSH_PATH)/tcsh

install: $(TARGET_PATH)/bin/tcsh

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(TCSH_PATH) clean
	rm -f $(TCSH_PATH)/.build-stamp $(TCSH_PATH)/.config-stamp

$(TCSH_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/tcsh-$(TCSH_VERSION).tar.gz
	touch $@

$(TCSH_PATH)/.patch-stamp: $(TCSH_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(TCSH_PATH) $(SOURCE_PATH)/tcsh '*.patch'
	touch $@

$(TCSH_PATH)/.config-stamp: $(TCSH_PATH)/.patch-stamp
	rm -f $(TCSH_PATH)/config.cache
	(cd $(TCSH_PATH);  \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./configure \
		--target=$(MFS_ARCH)-linux-uclibc \
		--host=$(MFS_ARCH)-linux-uclibc \
		--build=$(MFS_ARCH)-linux-gnu \
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

$(TCSH_PATH)/tcsh: $(TCSH_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
		STAGING_DIR=$(STAGING_DIR) \
	        $(MAKE) -C $(TCSH_PATH) \
		$(TARGET_CONFIGURE_OPTS) LD=$(MFS_ARCH)-linux-uclibc-gcc
	touch $@

$(TARGET_PATH)/bin/tcsh: $(TCSH_PATH)/tcsh
	install -d -m 755 $(TARGET_PATH)/bin
	rm -f $(TARGET_PATH)/bin/tcsh
	rm -f $(TARGET_PATH)/bin/csh
	install -m 755 $< $@
	$(STRIPCMD) --strip-unneeded $@
	ln -sf /bin/tcsh $(TARGET_PATH)/bin/csh
	touch -c $@

