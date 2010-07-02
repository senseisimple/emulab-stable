include ../../variables.mk

TEMPLATE_PATH	=	$(SOURCE_PATH)/target_template/template

.PHONY: all install clean

all:

clean:

install:
	mkdir -p $(TARGET_PATH)
	cp -dpR $(TEMPLATE_PATH)/* $(TARGET_PATH)
	find $(TARGET_PATH) -type d -name CVS | xargs rm -rf
	mkdir -p $(TARGET_PATH)/root/.ssh
	mkdir -p $(TARGET_PATH)/var/{tmp,log,lock,run,lib,state,cache,spool}
	mkdir -p $(TARGET_PATH)/tmp
	mkdir -p $(TARGET_PATH)/proc
	mkdir -p $(TARGET_PATH)/sys
	mkdir -p $(TARGET_PATH)/mnt
	mkdir -p $(TARGET_PATH)/home
	mkdir -p $(TARGET_PATH)/dev
