#!/bin/sh
#
# A temp Emulab patch until samba port is updated; make sure ldconfig is
# run before samba tries to start.
#

# PROVIDE: silly
# REQUIRE: ldconfig
# BEFORE: nmbd smbd
# KEYWORD: FreeBSD shutdown

exit 0
