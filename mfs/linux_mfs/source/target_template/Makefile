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
	mkdir -p $(TARGET_PATH)/var/tmp
	mkdir -p $(TARGET_PATH)/tmp
