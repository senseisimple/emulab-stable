#
# XXX ONLY RUN THIS INSTALL ON A LINUX TESTBED NODE!
#
# Trivial. These things just need to be installed into the right place
# on a testbed node before cutting an image.
#
#
INSTALL_DIR	= /etc/rc.d/testbed
INSTALL_FILES	= group gshadow passwd rc.testbed setup shadow
INSTALL		= /usr/bin/install -c 

install:
	-mkdir -p $(INSTALL_DIR)
	$(INSTALL) -m 644 group $(INSTALL_DIR)/group
	$(INSTALL) -m 644 passwd $(INSTALL_DIR)/passwd
	$(INSTALL) -m 600 shadow $(INSTALL_DIR)/shadow
	$(INSTALL) -m 600 gshadow $(INSTALL_DIR)/gshadow
	$(INSTALL) -m 755 rc.testbed $(INSTALL_DIR)/rc.testbed
	$(INSTALL) -m 755 setup $(INSTALL_DIR)/setup
	$(INSTALL) -m 755 ../tmcc $(INSTALL_DIR)/tmcc
