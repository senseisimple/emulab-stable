/*
 *
 */
#include "tbdefs.h"

char *tbdb_objecttypes[] = {
	TBDB_OBJECTTYPE_TESTBED,
	TBDB_OBJECTTYPE_LINK,
	TBDB_OBJECTTYPE_TRAFGEN,
	TBDB_OBJECTTYPE_TIME,
	0,
};

char *tbdb_eventtypes[] = {
	TBDB_EVENTTYPE_START,
	TBDB_EVENTTYPE_STOP,
	TBDB_EVENTTYPE_ISUP,
	TBDB_EVENTTYPE_REBOOT,
	TBDB_EVENTTYPE_UP,
	TBDB_EVENTTYPE_DOWN,
	TBDB_EVENTTYPE_MODIFY,
	TBDB_EVENTTYPE_SET,
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
