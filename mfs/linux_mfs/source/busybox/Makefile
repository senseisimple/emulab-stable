include ../../variables.mk

BUSYBOX_CONFIG		=	$(SOURCE_PATH)/busybox/busybox.config
BUSYBOX_VERSION		=	1.13.1
BUSYBOX_PATH		=	$(TARGET_BUILD_PATH)/busybox-$(BUSYBOX_VERSION)

.PHONY: extract patch config busybox
.PHONY: all install clean

all: busybox

install: $(TARGET_PATH)/bin/busybox

clean:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(BUSYBOX_PATH) clean
	rm -f $(BUSYBOX_PATH)/.build-stamp $(BUSYBOX_PATH)/.config-stamp

extract: $(BUSYBOX_PATH)/.extract-stamp

patch: $(BUSYBOX_PATH)/.patch-stamp

config: $(BUSYBOX_PATH)/.config-stamp

busybox: $(BUSYBOX_PATH)/busybox

menuconfig:
		$(MAKE) -C $(BUSYBOX_PATH) menuconfig

$(BUSYBOX_PATH)/.extract-stamp:
	mkdir -p $(TARGET_BUILD_PATH)
	cd $(TARGET_BUILD_PATH); tar xjf $(TARBALL_PATH)/busybox-$(BUSYBOX_VERSION).tar.bz2
	touch $@

$(BUSYBOX_PATH)/.patch-stamp: $(BUSYBOX_PATH)/.extract-stamp
	$(SCRIPTS_PATH)/patch-kernel.sh $(BUSYBOX_PATH) $(SOURCE_PATH)/busybox '*.patch'
	touch $@

$(BUSYBOX_PATH)/.config-stamp: $(BUSYBOX_PATH)/.patch-stamp
	sed "s,^CONFIG_PREFIX.*$$,CONFIG_PREFIX=\"$(TARGET_PATH)/\"," $(BUSYBOX_CONFIG) > \
		$(BUSYBOX_PATH)/.config
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(BUSYBOX_PATH) oldconfig
	touch $@


$(BUSYBOX_PATH)/busybox: $(BUSYBOX_PATH)/.config-stamp
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(BUSYBOX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX)
	touch $@

$(TARGET_PATH)/bin/busybox: $(BUSYBOX_PATH)/busybox
	mkdir -p $(TARGET_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) $(MAKE) -C $(BUSYBOX_PATH) CROSS_COMPILE=$(CROSS_COMPILER_PREFIX) install
