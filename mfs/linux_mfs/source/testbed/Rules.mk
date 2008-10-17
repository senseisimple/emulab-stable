.PHONY: imagezip-build frisbee-build tmcc-build growdisk-build

.PHONY: imagezip-clean frisbee-clean tmcc-clean growdisk-clean

.PHONY: imagezip-install frisbee-install tmcc-install growdisk-install

TESTBED_PATH = /home/rdjackso/testbed
TMCD_PATH = $(TESTBED_PATH)/tmcd
TESTBEDOBJ_PATH = /home/rdjackso/testbed-obj
FRISBEE_PATH = $(TESTBED_PATH)/os/frisbee.redux
IMAGEZIP_PATH = $(TESTBED_PATH)/os/imagezip
TMCD_OBJ_PATH = $(TESTBEDOBJ_PATH)/tmcd
TMCC_OBJ_PATH = $(TARGET_BUILD_PATH)/tmcc
CFLAGS = -Os

tmcc-build: $(TMCC_OBJ_PATH)/tmcc

imagezip-build: $(TESTBED_PATH)/os/imagezip/imageunzip

frisbee-build: $(FRISBEE_PATH)/frisbee

growdisk-build: $(TESTBED_PATH)/os/growdisk/growdisk

tmcc-clean:
	rm -rf $(TMCC_OBJ_PATH)

imagezip-clean:
	$(MAKE) -C $(TESTBED_PATH)/os/imagezip -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc clean

frisbee-clean:
	$(MAKE) -C $(FRISBEE_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc clean

growdisk-clean:
	rm -f $(TESTBED_PATH)/os/growdisk/growdisk

tmcc-install: $(TARGET_PATH)/usr/bin/tmcc

script-install:
	mkdir -p $(TARGET_PATH)/etc/testbed
	mkdir -p $(TARGET_PATH)/etc/emulab
	install -m 755 $(TMCD_PATH)/linux/control_interface $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/rc.frisbee $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/rc.ipod $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/slicefix $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/freebsd_to_linux_disk $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/check_disklabel $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/common/paths.sh $(TARGET_PATH)/etc/emulab


#imagezip-install: $(TARGET_PATH)/usr/bin/imageunzip $(TARGET_PATH)/usr/bin/imagezip
imagezip-install: $(TARGET_PATH)/usr/bin/imageunzip

frisbee-install: $(TARGET_PATH)/usr/bin/frisbee

growdisk-install: $(TARGET_PATH)/usr/bin/growdisk

$(TESTBED_PATH)/os/growdisk/growdisk:
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
		-o $@ \
		-I $(TESTBED_PATH)/os/imagezip \
		-Os \
		$(TESTBED_PATH)/os/growdisk/growdisk.c
	$(STRIPCMD) --strip-unneeded $@

# ARGH! tmcc wants to link against libtb just for the errorc function in libtb/log.c.
# libtb in turn wants libmysql.  There's no point building all of libtb and its
# required libs under uclibc, so I'm going to skip the testbed makefiles and
# build tmcc here.  Note that you must still configure the object tree with the
# tbdefs file you want, since we need config.h.

$(TMCC_OBJ_PATH)/tmcc: $(SYSROOT_OPENSSL_SHARED)
	mkdir -p $(TMCC_OBJ_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
		-o $@ \
		-I$(TESTBEDOBJ_PATH) \
		-I$(TESTBED_PATH)/lib/libtb \
		-Os \
		-lssl \
		$(TESTBED_PATH)/tmcd/tmcc.c \
		$(TESTBED_PATH)/tmcd/ssl.c \
		$(TESTBED_PATH)/lib/libtb/log.c
	$(STRIPCMD) --strip-unneeded $@

#	$(MAKE) -C $(TMCD_PATH) \
#		CC=$(CROSS_COMPILER_PREFIX)gcc \
#		CFLAGS="$(CFLAGS) -I$(TESTBEDOBJ_PATH)" \
#		PATH=$(STAGING_DIR)/usr/bin:$(PATH) tmcc

$(FRISBEE_PATH)/frisbee: $(SYSROOT_ZLIB_SHARED)
	$(MAKE) -C $(FRISBEE_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) frisbee
	$(STRIPCMD) --strip-unneeded $@

$(IMAGEZIP_PATH)/imageunzip: $(SYSROOT_ZLIB_SHARED)
	$(MAKE) -C $(IMAGEZIP_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) imageunzip
	$(STRIPCMD) --strip-unneeded $@

$(IMAGEZIP_PATH)/imagezip: $(SYSROOT_ZLIB_SHARED)
	$(MAKE) -C $(IMAGEZIP_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) imagezip
	$(STRIPCMD) --strip-unneeded $@

$(TARGET_PATH)/usr/bin/growdisk: $(TESTBED_PATH)/os/growdisk/growdisk
	mkdir -p $(TARGET_PATH)/usr/bin
	cp $< $(TARGET_PATH)/usr/bin/growdisk
	touch $@

$(TARGET_PATH)/usr/bin/imageunzip: $(IMAGEZIP_PATH)/imageunzip
	mkdir -p $(TARGET_PATH)/usr/bin
	cp $(IMAGEZIP_PATH)/imageunzip $(TARGET_PATH)/usr/bin
	touch $@

$(TARGET_PATH)/usr/bin/tmcc: $(TMCC_OBJ_PATH)/tmcc
	mkdir -p $(TARGET_PATH)/usr/bin
	cp $< $@
	touch $@

$(TARGET_PATH)/usr/bin/imagezip: $(IMAGEZIP_PATH)/imagezip
	mkdir -p $(TARGET_PATH)/usr/bin
	cp $(IMAGEZIP_PATH)/imagezip $(TARGET_PATH)/usr/bin
	touch $@

$(TARGET_PATH)/usr/bin/frisbee: $(FRISBEE_PATH)/frisbee
	mkdir -p $(TARGET_PATH)/usr/bin
	cp $(FRISBEE_PATH)/frisbee $(TARGET_PATH)/usr/bin
	touch $@
