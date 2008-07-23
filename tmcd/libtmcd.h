#ifndef _LIBTMCD_H_
#define _LIBTMCD_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include "tbdefs.h"

/*
 * This structure is passed to each request function. The intent is to
 * reduce the number of DB queries per request to a minimum.
 */
typedef struct {
	int		version;
	struct in_addr  client;
	int		allocated;
	int		jailflag;
	int		isvnode;
	int		issubnode;
	int		islocal;
	int		isssl;
	int		istcp;
	int		iscontrol;
	int		isplabdslice;
	int		isplabsvc;
	int		elab_in_elab;
        int		singlenet;	  /* Modifier for elab_in_elab */
	int		update_accounts;
	int		exptidx;
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
} tmcdreq_t;

typedef struct {
	char		*data;
	int		length;
} tmcdresp_t;

int	iptonodeid(struct in_addr, tmcdreq_t *);
int	nodeidtoexp(char *nodeid, char *pid, char *eid, char *gid);
int	checkprivkey(struct in_addr, char *);
int	checkdbredirect(tmcdreq_t *);
void	tmcd_free_response(tmcdresp_t *);
tmcdresp_t *tmcd_handle_request(int, char *, char *, tmcdreq_t *);
int tmcd_init(char *, struct in_addr *, int, int);

#endif /* _LIBTMCD_H_ */
