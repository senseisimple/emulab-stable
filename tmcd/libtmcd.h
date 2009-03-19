#ifndef _LIBTMCD_H_
#define _LIBTMCD_H_

#include <netinet/in.h>
#include <mysql/mysql.h>
#include <arpa/inet.h>
#include "tbdefs.h"

#ifdef EVENTSYS
#include "event.h"
#endif

#define DBNAME_SIZE	64
#define HOSTID_SIZE	(32+64)

#define TMCD_STATUS_OK				0
#define TMCD_STATUS_UNKNOWN_COMMAND		1
#define TMCD_STATUS_INVALID_UDP_REQUEST		2
#define TMCD_STATUS_INVALID_REMOTE_UDP_REQUEST	3
#define TMCD_STATUS_REQUIRES_ENCRYPTION		4
#define TMCD_STATUS_NODE_NOT_ALLOCATED		5
#define TMCD_STATUS_MALLOC_FAILED		6
#define TMCD_STATUS_COMMAND_FAILED		7

/*
 * This structure is passed to each request function. The intent is to
 * reduce the number of DB queries per request to a minimum.
 */
typedef struct {
	MYSQL		db;
	int		db_connected;
	char		dbname[DBNAME_SIZE];
	char		prev_dbname[DBNAME_SIZE];
	struct in_addr	myipaddr;
#ifdef EVENTSYS
	event_handle_t	event_handle;
#endif
	char		fshostid[HOSTID_SIZE];
	int		verbose;
	int		debug;
	int		isssl;
	int		istcp;
	int		version;
	struct in_addr  client;
	int		allocated;
	int		jailflag;
	int		isvnode;
	int		issubnode;
	int		islocal;
	int		isdedicatedwa;
	int		iscontrol;
	int		isplabdslice;
	int		isplabsvc;
	int		elab_in_elab;
        int		singlenet;	  /* Modifier for elab_in_elab */
	int		update_accounts;
	int		exptidx;
	int		creator_idx;
	int		swapper_idx;
	int		swapper_isadmin;
	int		genisliver_idx;
	char		nodeid[TBDB_FLEN_NODEID];
	char		vnodeid[TBDB_FLEN_NODEID];
	char		pnodeid[TBDB_FLEN_NODEID]; /* XXX */
	char		pid[TBDB_FLEN_PID];
	char		eid[TBDB_FLEN_EID];
	char		gid[TBDB_FLEN_GID];
	char		nickname[TBDB_FLEN_VNAME];
	char		type[TBDB_FLEN_NODETYPE];
	char		class[TBDB_FLEN_NODECLASS];
        char		ptype[TBDB_FLEN_NODETYPE];	/* Of physnode */
	char		pclass[TBDB_FLEN_NODECLASS];	/* Of physnode */
	char		creator[TBDB_FLEN_UID];
	char		swapper[TBDB_FLEN_UID];
	char		syncserver[TBDB_FLEN_VNAME];	/* The vname */
	char		keyhash[TBDB_FLEN_PRIVKEY];
	char		eventkey[TBDB_FLEN_PRIVKEY];
	char		sfshostid[TBDB_FLEN_SFSHOSTID];
	char		testdb[TBDB_FLEN_TINYTEXT];
	char		tmcd_redirect[TBDB_FLEN_TINYTEXT];
	char		privkey[TBDB_FLEN_PRIVKEY+1];
} tmcdreq_t;

typedef struct {
	char		*data;
	char		*type;
	int		length;
} tmcdresp_t;

int	iptonodeid(tmcdreq_t *reqp, struct in_addr ipaddr, char* nodekey);
int	checkprivkey(tmcdreq_t *reqp, struct in_addr, char *);
int	checkdbredirect(tmcdreq_t *);
void	tmcd_free_response(tmcdresp_t *);
int tmcd_handle_request(tmcdreq_t *, tmcdresp_t **, char *, char *);
int tmcd_init(tmcdreq_t *reqp, struct in_addr *, char *);

#endif /* _LIBTMCD_H_ */
