/*
 * Testbed DB interface and support.
 */
#include <stdarg.h>
#include <mysql/mysql.h>

/*
 * Generic interface.
 */
int		dbinit(void);

/*
 * TB functions.
 */
int	mydb_iptonodeid(char *ipaddr, char *bufp);
int	mydb_setnodeeventstate(char *nodeid, char *eventtype);
int	mydb_checkexptnodeeventstate(char *pid, char *eid,char *ev,int *count);
int	mydb_seteventschedulerpid(char *pid, char *eid, int processid);

/*
 * mysql specific routines.
 */
MYSQL_RES      *mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);

/*
 * Various constants.
 */
#define	TBDB_FLEN_NODEID	64
#define TBDB_FLEN_EVOBJTYPE	128
#define TBDB_FLEN_EVOBJNAME	128
#define TBDB_FLEN_EVEVENTTYPE	128

/*
 * Event system stuff
 */
#define TBDB_OBJECTTYPE_TESTBED	"TBCONTROL"
#define TBDB_OBJECTTYPE_LINK	"LINK"
#define TBDB_OBJECTTYPE_TRAFGEN	"TRAFGEN"

#define TBDB_EVENTTYPE_ISUP	"ISUP"
#define TBDB_EVENTTYPE_REBOOT	"REBOOT"
#define TBDB_EVENTTYPE_UP	"UP"
#define TBDB_EVENTTYPE_DOWN	"DOWN"
#define TBDB_EVENTTYPE_MODIFY	"MODIFY"
