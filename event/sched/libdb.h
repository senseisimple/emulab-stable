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
 * mysql specific routines.
 */
MYSQL_RES      *mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);
