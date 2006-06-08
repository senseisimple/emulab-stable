/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Testbed DB interface and support.
 */
#include <stdarg.h>
#include <mysql/mysql.h>
#include "tbdefs.h"

/*
 * Generic interface.
 */
int		dbinit(void);
int		dbinit_withparams(char *host,
				  char *user, char *passwd, char *name);
void		dbclose(void);

/*
 * TB functions.
 */
int	mydb_iptonodeid(char *ipaddr, char *bufp);
int	mydb_nodeidtoip(char *nodeid, char *bufp);
int	mydb_setnodeeventstate(char *nodeid, char *eventtype);
int	mydb_checkexptnodeeventstate(char *pid, char *eid,char *ev,int *count);
int	mydb_seteventschedulerpid(char *pid, char *eid, int processid);

/*
 * mysql specific routines.
 */
MYSQL_RES      *mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);

