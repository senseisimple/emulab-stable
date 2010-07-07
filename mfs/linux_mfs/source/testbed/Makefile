include ../../variables.mk

BUILDROOT	:=	$(PWD)/$(BUILDROOT)

PROGS	:= imageunzip imagezip frisbee growdisk tmcc.bin binoffset groklilo teachswitch dhcp_packet_dump
BUILD_TARGETS := $(PROGS)
INSTALL_TARGETS := $(addsuffix -install,$(PROGS))

.PHONY: clean
.PHONY: $(BUILD_TARGETS) all
.PHONY: $(INSTALL_TARGETS) script-install install

TESTBED_PATH = /home/rdjackso/gits/testbed
TESTBED_BUILD_PATH = $(TARGET_BUILD_PATH)/testbed
TMCD_PATH = $(TESTBED_PATH)/tmcd
GROKLILO_PATH = $(TESTBED_PATH)/cdrom/groklilo
GROWDISK_PATH = $(TESTBED_PATH)/os/growdisk
TESTBEDOBJ_PATH = /home/rdjackso/testbed-obj
FRISBEE_PATH = $(TESTBED_PATH)/os/frisbee.redux
BINOFFSET_PATH = $(TESTBED_PATH)/tools/binoffset
IMAGEZIP_PATH = $(TESTBED_PATH)/os/imagezip
TEACHSWITCH_PATH = $(TESTBED_PATH)/tools/teachswitch
DHCP_PACKET_DUMP_PATH = $(TESTBED_PATH)/tools/dhcp_packet_dump
CFLAGS = -Os

INSTALLED_BINS	:= $(addprefix $(TARGET_PATH)/usr/, \
			$(addprefix bin/, binoffset imagezip imageunzip tmcc.bin frisbee teachswitch dhcp_packet_dump) \
			$(addprefix sbin/, groklilo growdisk))

all: $(BUILD_TARGETS)

# Use GNU make's second-expansion feature to simplify the installed binary targets.
# This target needs to be declared here, so do not move it.
.SECONDEXPANSION:

$(BUILD_TARGETS): $(TESTBED_BUILD_PATH)/$$@

# Install all of the compiled binaries.  Put them in sane locations like
# /usr/bin and /usr/sbin, but put symlinks for them in $BINDIR for
# compatibility.
$(INSTALLED_BINS): $(TESTBED_BUILD_PATH)/$$(notdir $$@)
	mkdir -p $(dir $@)
	install -m 755 $< $@
	mkdir -p $(TARGET_PATH)/etc/testbed
	ln -sf $(subst $(TARGET_PATH),,$@) $(TARGET_PATH)/etc/testbed

script-install:
	mkdir -p $(TARGET_PATH)/etc/testbed
	mkdir -p $(TARGET_PATH)/etc/testbed/rc
	mkdir -p $(TARGET_PATH)/etc/emulab
	mkdir -p $(TARGET_PATH)/usr/bin
	mkdir -p $(TARGET_PATH)/usr/local/etc
	mkdir -p $(TARGET_PATH)/var/emulab/logs
	mkdir -p $(TARGET_PATH)/var/emulab/lock
	mkdir -p $(TARGET_PATH)/var/emulab/boot
	mkdir -p $(TARGET_PATH)/var/emulab/db
	mkdir -p $(TARGET_PATH)/usr/share/udhcpc
	ln -sf /etc/testbed $(TARGET_PATH)/usr/local/etc/emulab
	install -m 755 $(TESTBEDOBJ_PATH)/tmcd/linux/sethostname.dhclient $(TARGET_PATH)/usr/share/udhcpc
	install -m 755 $(TMCD_PATH)/linux/control_interface $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/rc.frisbee $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/linux/rc.ipod $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/linux/slicefix $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/freebsd_to_linux_disk $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/mount_bsd_slice $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/check_disklabel $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/remap_ide_disks $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/get_edd_map.pl $(TARGET_PATH)/etc/testbed
	#install -m 755 $(TMCD_PATH)/linux/extract_kernel_info $(TARGET_PATH)/etc/testbed
	#install -m 755 $(TMCD_PATH)/linux/extract_image_info $(TARGET_PATH)/etc/testbed
	#install -m 755 $(TMCD_PATH)/linux/extract_initrd_info $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/linux_slicefix.pl $(TARGET_PATH)/etc/testbed
	install -m 755 $(TESTBED_PATH)/install/newclient $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/common/paths.sh $(TARGET_PATH)/etc/emulab
	install -m 755 $(TMCD_PATH)/common/paths.pm $(TARGET_PATH)/etc/emulab
	install -m 644 $(TMCD_PATH)/common/libsetup.pm $(TARGET_PATH)/etc/testbed
	install -m 644 $(TMCD_PATH)/common/libtestbed.pm $(TARGET_PATH)/etc/testbed
	install -m 644 $(TMCD_PATH)/common/libtmcc.pm $(TARGET_PATH)/etc/testbed
	install -m 644 $(TMCD_PATH)/linux/liblocsetup.pm $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/common/watchdog $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/common/update $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/common/tmcc.pl $(TARGET_PATH)/usr/bin/tmcc
	install -m 755 $(TMCD_PATH)/common/osconfig $(TARGET_PATH)/etc/testbed
	ln -sf /usr/bin/tmcc $(TARGET_PATH)/etc/testbed/tmcc
	install -m 755 $(TMCD_PATH)/common/config/librc.pm $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/common/config/rc.config $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/linux/rc.mfs $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.misc $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.localize $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.mounts $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.accounts $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.hostnames $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.keys $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/config/rc.startcmd $(TARGET_PATH)/etc/testbed/rc
	install -m 755 $(TMCD_PATH)/common/runstartup $(TARGET_PATH)/etc/testbed
	install -m 755 $(TMCD_PATH)/linux/create-image $(TARGET_PATH)/etc/testbed
	mkdir -p $(TARGET_PATH)/usr/local/bin
	ln -sf /usr/bin/imagezip $(TARGET_PATH)/usr/local/bin
	ln -sf /etc/testbed/create-image $(TARGET_PATH)/usr/local/bin
	touch $(TARGET_PATH)/etc/emulab/ismfs

imageunzip-install: $(TARGET_PATH)/usr/bin/imageunzip

imagezip-install: $(TARGET_PATH)/usr/bin/imagezip

frisbee-install: $(TARGET_PATH)/usr/bin/frisbee

growdisk-install: $(TARGET_PATH)/usr/sbin/growdisk

binoffset-install: $(TARGET_PATH)/usr/bin/binoffset

groklilo-install: $(TARGET_PATH)/usr/sbin/groklilo

tmcc.bin-install: $(TARGET_PATH)/usr/bin/tmcc.bin

teachswitch-install: $(TARGET_PATH)/usr/bin/teachswitch

dhcp_packet_dump-install: $(TARGET_PATH)/usr/bin/dhcp_packet_dump

clean:
		rm -rf $(TESTBED_BUILD_PATH)

install: $(INSTALL_TARGETS) script-install

$(TESTBED_BUILD_PATH)/groklilo:
	mkdir -p $(TESTBED_BUILD_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
	-o $@ \
	$(GROKLILO_PATH)/groklilo.c
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/binoffset:
	mkdir -p $(TESTBED_BUILD_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
	-o $@ \
	$(BINOFFSET_PATH)/binoffset.c
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/growdisk:
	mkdir -p $(TESTBED_BUILD_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
		-o $@ \
		-I $(TESTBED_PATH)/os/imagezip \
		-Os \
		$(GROWDISK_PATH)/growdisk.c
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/teachswitch:
	mkdir -p $(TESTBED_BUILD_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
		-o $@ \
		-Os \
		$(TEACHSWITCH_PATH)/teachswitch.c
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/dhcp_packet_dump:
	mkdir -p $(TESTBED_BUILD_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
		-o $@ \
		-Os \
		-I$(DHCP_PACKET_DUMP_PATH) \
		$(DHCP_PACKET_DUMP_PATH)/dhcp_packet_dump.c
	$(STRIPCMD) --strip-unneeded $@

# ARGH! tmcc wants to link against libtb just for the errorc function in libtb/log.c.
# libtb in turn wants libmysql.  There's no point building all of libtb and its
# required libs under uclibc, so I'm going to skip the testbed makefiles and
# build tmcc here.

$(TESTBED_BUILD_PATH)/tmcc.bin:
	mkdir -p $(TESTBED_BUILD_PATH)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(CROSS_COMPILER_PREFIX)gcc \
		-o $@ \
		-I. \
		-I$(TESTBED_PATH)/lib/libtb \
		-Os \
		$(TESTBED_PATH)/tmcd/tmcc.c \
		$(TESTBED_PATH)/lib/libtb/log.c
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/frisbee: $(SYSROOT_ZLIB_SHARED)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(MAKE) -C $(FRISBEE_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) frisbee
	mkdir -p $(dir $@)
	mv $(FRISBEE_PATH)/frisbee $@
	$(MAKE) -C $(FRISBEE_PATH) -f Makefile-linux.sa clean
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/imageunzip: $(SYSROOT_ZLIB_SHARED)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(MAKE) -C $(IMAGEZIP_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc \
		imageunzip
	mkdir -p $(dir $@)
	mv $(IMAGEZIP_PATH)/imageunzip $@
	$(MAKE) -C $(IMAGEZIP_PATH) -f Makefile-linux.sa clean
	$(STRIPCMD) --strip-unneeded $@

$(TESTBED_BUILD_PATH)/imagezip: $(SYSROOT_ZLIB_SHARED)
	PATH=$(STAGING_DIR)/usr/bin:$(PATH) \
	$(MAKE) -C $(IMAGEZIP_PATH) -f Makefile-linux.sa \
		CC=$(CROSS_COMPILER_PREFIX)gcc \
		PATH=$(STAGING_DIR)/usr/bin:$(PATH) imagezip
	mkdir -p $(dir $@)
	mv $(IMAGEZIP_PATH)/imagezip $@
	$(MAKE) -C $(IMAGEZIP_PATH) -f Makefile-linux.sa clean
	$(STRIPCMD) --strip-unneeded $@

