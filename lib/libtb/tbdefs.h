/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Various constants that are reflected in the DB!
 */
#define	TBDB_FLEN_NODEID	(12 + 1)
#define	TBDB_FLEN_VNAME		(32 + 1)
#define	TBDB_FLEN_EID		(32 + 1)
#define	TBDB_FLEN_PID		(12 + 1)
#define	TBDB_FLEN_GID		(16 + 1)
#define	TBDB_FLEN_NODECLASS	(10 + 1)
#define	TBDB_FLEN_NODETYPE	(30 + 1)
#define TBDB_FLEN_EVOBJTYPE	128
#define TBDB_FLEN_EVOBJNAME	128
#define TBDB_FLEN_EVEVENTTYPE	128
#define TBDB_FLEN_PRIVKEY	64

/*
 * Event system stuff.
 *
 * If you add to these two lists, make sure you add to the arrays in tbdefs.c
 */
#define TBDB_OBJECTTYPE_TESTBED	"TBCONTROL"
#define TBDB_OBJECTTYPE_LINK	"LINK"
#define TBDB_OBJECTTYPE_TRAFGEN	"TRAFGEN"
#define TBDB_OBJECTTYPE_TIME	"TIME"
#define TBDB_OBJECTTYPE_PROGRAM	"PROGRAM"

#define TBDB_EVENTTYPE_START	"START"
#define TBDB_EVENTTYPE_STOP	"STOP"
#define TBDB_EVENTTYPE_KILL	"KILL"
#define TBDB_EVENTTYPE_ISUP	"ISUP"
#define TBDB_EVENTTYPE_REBOOT	"REBOOT"
#define TBDB_EVENTTYPE_UP	"UP"
#define TBDB_EVENTTYPE_DOWN	"DOWN"
#define TBDB_EVENTTYPE_MODIFY	"MODIFY"
#define TBDB_EVENTTYPE_SET	"SET"
#define TBDB_EVENTTYPE_RESET	"RESET"

/*
 * Protos.
 */
int	tbdb_validobjecttype(char *foo);
int	tbdb_valideventtype(char *foo);
