/*
 * Various constants that are reflected in the DB!
 */
#define	TBDB_FLEN_NODEID	64
#define	TBDB_FLEN_VNAME		32
#define	TBDB_FLEN_EID		32
#define	TBDB_FLEN_PID		12
#define	TBDB_FLEN_GID		16
#define TBDB_FLEN_EVOBJTYPE	128
#define TBDB_FLEN_EVOBJNAME	128
#define TBDB_FLEN_EVEVENTTYPE	128

/*
 * Event system stuff
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

/*
 * Protos.
 */
int	tbdb_validobjecttype(char *foo);
int	tbdb_valideventtype(char *foo);
