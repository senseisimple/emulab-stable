/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 *
 */
#include <string.h>
#include "tbdefs.h"

char *tbdb_objecttypes[] = {
	TBDB_OBJECTTYPE_TESTBED,
	TBDB_OBJECTTYPE_STATE,
	TBDB_OBJECTTYPE_OPMODE,
	TBDB_OBJECTTYPE_LINK,
	TBDB_OBJECTTYPE_TRAFGEN,
	TBDB_OBJECTTYPE_TIME,
	TBDB_OBJECTTYPE_PROGRAM,
	TBDB_OBJECTTYPE_FRISBEE,
	TBDB_OBJECTTYPE_SIMULATOR,
	0,
};

char *tbdb_eventtypes[] = {
	TBDB_EVENTTYPE_START,
	TBDB_EVENTTYPE_STOP,
	TBDB_EVENTTYPE_KILL,
	TBDB_EVENTTYPE_ISUP,
	TBDB_EVENTTYPE_REBOOT,
	TBDB_EVENTTYPE_UP,
	TBDB_EVENTTYPE_DOWN,
	TBDB_EVENTTYPE_MODIFY,
	TBDB_EVENTTYPE_SET,
	TBDB_EVENTTYPE_RESET,
	TBDB_EVENTTYPE_HALT,
	TBDB_EVENTTYPE_SWAPOUT,
        TBDB_NODESTATE_ISUP, 
        TBDB_NODESTATE_REBOOTED, 
        TBDB_NODESTATE_REBOOTING, 
        TBDB_NODESTATE_SHUTDOWN, 
        TBDB_NODESTATE_BOOTING, 
        TBDB_NODESTATE_TBSETUP, 
        TBDB_NODESTATE_RELOADSETUP, 
        TBDB_NODESTATE_RELOADING, 
        TBDB_NODESTATE_RELOADDONE, 
        TBDB_NODESTATE_UNKNOWN, 
        TBDB_NODEOPMODE_NORMAL, 
        TBDB_NODEOPMODE_DELAYING, 
        TBDB_NODEOPMODE_UNKNOWNOS, 
        TBDB_NODEOPMODE_RELOADING, 
        TBDB_NODEOPMODE_NORMALv1, 
        TBDB_NODEOPMODE_MINIMAL, 
        TBDB_NODEOPMODE_RELOAD, 
        TBDB_NODEOPMODE_DELAY, 
        TBDB_NODEOPMODE_BOOTWHAT, 
        TBDB_NODEOPMODE_UNKNOWN, 
        TBDB_TBCONTROL_RESET, 
        TBDB_TBCONTROL_RELOADDONE, 
        TBDB_TBCONTROL_TIMEOUT, 
    	0,
};

/*
 * Check that events are legal.
 */
int
tbdb_validobjecttype(char *foo)
{
	char	**bp = tbdb_objecttypes;

	while (*bp) {
		if (!strcmp(*bp, foo))
			return 1;
		bp++;
	}
	return 0;
}

int
tbdb_valideventtype(char *foo)
{
	char	**bp = tbdb_eventtypes;

	while (*bp) {
		if (!strcmp(*bp, foo))
			return 1;
		bp++;
	}
	return 0;
}
