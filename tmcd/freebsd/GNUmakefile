#
# XXX ONLY RUN THIS INSTALL ON A FREEBSD TESTBED NODE!
#
# Trivial. These things just need to be installed into the right place
# on a testbed node before cutting an image.
#
#
INSTALL_DIR	= /etc/testbed
INSTALL_FILES	= group master.passwd setup
INSTALL		= /usr/bin/install -c 

install:
	-mkdir -p $(INSTALL_DIR)
	$(INSTALL) -m 644 group $(INSTALL_DIR)/group
	$(INSTALL) -m 600 master.passwd $(INSTALL_DIR)/master.passwd
	$(INSTALL) -m 755 setup $(INSTALL_DIR)/setup
	$(INSTALL) -m 755 ../tmcc $(INSTALL_DIR)/tmcc
