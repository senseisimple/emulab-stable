include ../../variables.mk

NCURSES_VERSION	=	5.7+20100313
NCURSES_PATH	=	$(TARGET_BUILD_PATH)/ncurses-$(NCURSES_VERSION)

.PHONY: extract patch config \
	ncurses install clean all

all: ncurses

extract: $(NCURSES_PATH)/.extract-stamp

patch: $(NCURSES_PATH)/.patch-stamp

config: $(NCURSES_PATH)/.config-stamp

ncurses: $(NCURSES_PATH)/lib/libncurses.a

install: $(STAGING_DIR)/lib/libncurses.a $(TARGET_PATH)/lib/libncurses.so.5.7

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(NCURSES_PATH) clean
	rm -f $(NCURSES_PATH)/.build-stamp $(NCURSES_PATH)/.config-stamp

$(NCURSES_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xzf $(TARBALL_PATH)/ncurses-$(NCURSES_VERSION).tar.gz
	touch $@

$(NCURSES_PATH)/.patch-stamp: $(NCURSES_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(NCURSES_PATH) $(SOURCE_PATH)/ncurses '*.patch'
	touch $@

$(NCURSES_PATH)/.config-stamp: $(NCURSES_PATH)/.patch-stamp
	rm -f $(NCURSES_PATH)/config.cache
	(cd $(NCURSES_PATH);  \
		CC=$(TARGET_CC) \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
		$(HOST_CONFIGURE_OPTS) \
		./configure \
		--target=$(MFS_ARCH)-linux-uclibc \
		--host=$(MFS_ARCH)-linux-uclibc \
		--build=$(MFS_ARCH)-linux-gnu \
		--prefix=$(STAGING_DIR) \
		--exec-prefix=/usr \
		--bindir=/usr/bin \
		--libdir=/lib \
		--libexecdir=/usr/lib \
		--sbindir=/usr/sbin \
		--sysconfdir=/etc \
		--datadir=/usr/share \
		--with-shared \
		--without-gpm \
		--without-cxx \
		--without-manpages \
		--without-progs \
		--without-cxx-binding \
		--localstatedir=/var \
		--mandir=/usr/man \
		--infodir=/usr/info \
	)
	touch $@

#--enable-elf-shlibs --enable-dynamic-e2fsck --disable-swapfs \

#--enable-elf-shlibs \

$(NCURSES_PATH)/lib/libncurses.a: $(NCURSES_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	        CC=$(TARGET_CC) \
		CXX=$(TARGET_CC) \
	        $(MAKE) -C $(NCURSES_PATH) \
		$(TARGET_CONFIGURE_OPTS)

$(TARGET_PATH)/lib/libncurses.so.5.7: $(STAGING_DIR)/lib/libncurses.so.5.7
	mkdir -p $(TARGET_PATH)/usr/share/terminfo
	mkdir -p $(TARGET_PATH)/usr/share/terminfo/l
	mkdir -p $(TARGET_PATH)/usr/share/terminfo/v
	mkdir -p $(TARGET_PATH)/usr/share/terminfo/s
	mkdir -p $(TARGET_PATH)/usr/share/terminfo/x
	mkdir -p $(TARGET_PATH)/usr/share/tabset
	cp -fa $(STAGING_DIR)/usr/share/tabset/* $(TARGET_PATH)/usr/share/tabset
	cp -fa $(STAGING_DIR)/usr/share/terminfo/v/vt{100,220} \
		$(TARGET_PATH)/usr/share/terminfo/v
	cp -fa $(STAGING_DIR)/usr/share/terminfo/l/linux \
		$(TARGET_PATH)/usr/share/terminfo/l
	cp -fa $(STAGING_DIR)/usr/share/terminfo/s/screen* \
		$(TARGET_PATH)/usr/share/terminfo/s
	cp -fa $(STAGING_DIR)/usr/share/terminfo/x/xterm \
		$(TARGET_PATH)/usr/share/terminfo/x
	mkdir -p $(TARGET_PATH)/lib
	cp -fa $(STAGING_DIR)/lib/libncurses*.so* $(TARGET_PATH)/lib
	$(STRIPCMD) $@
	touch -c $@

$(STAGING_DIR)/lib/libncurses.a: $(NCURSES_PATH)/lib/libncurses.a
	$(MAKE) -C $(NCURSES_PATH)/include install
	(cd $(NCURSES_PATH)/misc; \
	prefix=$(STAGING_DIR) \
	exec_prefix=/usr \
	bindir=/usr/bin \
	top_srcdir=.. \
	srcdir=. \
	datadir=/usr/share \
	ticdir=$(STAGING_DIR)/usr/share/terminfo \
	source=terminfo.tmp \
	cross_compiling=yes \
	/bin/sh ./run_tic.sh)
	mkdir -p $(STAGING_DIR)/usr/share/tabset
	(cd $(NCURSES_PATH)/misc/tabset; \
	/bin/sh -c 'for i in * ; do \
		if test -f $$i ; then \
			echo installing $$i; \
			/usr/bin/install -c -m 644 $$i $(STAGING_DIR)/usr/share/tabset/$$i; \
		fi; done' \
	)

	cp -dp $(NCURSES_PATH)/lib/lib* $(STAGING_DIR)/lib
