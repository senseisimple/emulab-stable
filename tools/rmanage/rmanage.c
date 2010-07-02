/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "rmcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#ifndef NO_EMULAB
#include <mysql/mysql.h>
#include "tbdb.h"
#endif

extern char *optarg;
extern int optopt;
extern int opterr;
extern int optind;

#define COMMAND_PING         "ping"
#define COMMAND_CAPABILITIES "capabilities"
#define COMMAND_STATE        "state"
#define COMMAND_NOP          "nop"
#define COMMAND_RESET        "reset"
#define COMMAND_POWER_CYCLE  "cycle"
#define COMMAND_POWER_DOWN   "off"
#define COMMAND_POWER_UP     "on"
#define COMMAND_COUNT        0x08

static char *commands[COMMAND_COUNT][2] = 
    {
	{ COMMAND_PING, "Send an RMCP ping and display supported modes." },
	{ COMMAND_CAPABILITIES, "Get and display RMCP capabilities." },
	{ COMMAND_STATE, "Get and display current node power state." },
	{ COMMAND_NOP, "Open a session if in secure mode; else nothing." },
	{ COMMAND_RESET, "Send a warm reset command." },
	{ COMMAND_POWER_CYCLE, "Send a full power cycle command." },
	{ COMMAND_POWER_DOWN, "Send a power down command." },
	{ COMMAND_POWER_UP, "Send a power up command." }
    };

#define DEFAULT_TIMEOUT 3
#define DEFAULT_RETRIES 3

void usage(char *prog) {
    int i;

    fprintf(stderr,
	    "Usage: %s [-hHdsE] [-tmrkgu <arg>] <clientname> <command>\n"
	    "\n"
	    "  -h         Print this message\n"
	    "  -d         Turn on debugging (more d's mean more debug info)\n"
	    "  -s         Use secure RMCP\n"
	    "  -H         Interpret keys as hex strings instead of char strings\n"
#ifndef NO_EMULAB
	    "  -E         Resolve client and keys using the Emulab database\n"
#endif
	    "               (this option implies -H)\n"
	    "  -t timeout Timeout (in seconds) for individual RMCP messages\n"
	    "               (default: %d)\n"
	    "  -m retries Retry N times for unacknowledged RMCP sends\n"
	    "               (default: %d)\n"
	    "  -r role    Use this role (either 'operator' or 'administrator')\n"
	    "               (default 'administrator')\n"
	    "  -k key     Use this key with the role specified by '-r'\n"
	    "  -g key     Use this generation key\n"
	    "  -u uid     Send the specified username\n"
	    "\n"
	    "  clientname IP or hostname\n"
#ifndef NO_EMULAB
	    "    (or Emulab node_id if -E specified)\n"
#endif
	    "  command    This argument performs an operation on the managed\n"
	    "             client.  The available commands are:\n",
	    prog,DEFAULT_TIMEOUT,DEFAULT_RETRIES);
    for (i = 0; i < COMMAND_COUNT; ++i) {
	fprintf(stderr,
		"    %s\t[%s]\n",
		commands[i][0],commands[i][1]);
    }
    fprintf(stderr,"\n");

    return;
}

int main(int argc,char **argv) {
    rmcp_error_t retval;
    rmcp_ctx_t *ctx;
    rmcp_asf_supported_t *supported;
    rmcp_asf_capabilities_t *capabilities;
    rmcp_asf_sysstate_t *sysstate;
    
    int c;
    int debug = 0;
    int secure = 0;
    int hex_mode = 0;
    char *client = NULL;
    u_int8_t *rkey = NULL;
    int rkey_len;
    u_int8_t *gkey = NULL;
    int gkey_len;
    int timeout = DEFAULT_TIMEOUT;
    int retries = DEFAULT_RETRIES;
    char *role = "administrator";
    int roleno;
    char *uid = NULL;
    char *command = NULL;
    int emulab = 0;
    char emip[16];
#ifndef NO_EMULAB
    MYSQL_RES *res;
    MYSQL_ROW row;
    int nrows;
#endif
    char *src;
    int clen;

    while ((c = getopt(argc,argv,"t:m:r:k:g:u:hdsHE")) != -1) {
	switch (c) {
	case 'h':
	    usage(argv[0]);
	    exit(0);
	    break;
	case 'd':
	    ++debug;
	    break;
	case 's':
	    secure = 1;
	    break;
	case 'H':
	    hex_mode = 1;
	    break;
	case 'E':
	    emulab = 1;
	    /* assume keys in the Emulab db are in hex mode... */
	    hex_mode = 1;
	    break;
	case 't':
	    timeout = atoi(optarg);
	    break;
	case 'm':
	    retries = atoi(optarg);
	    break;
	case 'r':
	    role = optarg;
	    if (strcmp("operator",role) && strcmp("administrator",role)) {
		fprintf(stderr,"ERROR: unknown role '%s'!\n",role);
		exit(-1);
	    }
	    break;
	case 'k':
/*  	    rkey = (u_int8_t *)malloc((strlen(optarg)-1)*sizeof(u_int8_t)); */
/*  	    memcpy(rkey,optarg,strlen(optarg)-1); */
/*  	    rkey_len = strlen(optarg)-1; */
	    rkey = optarg;
	    rkey_len = strlen(optarg);
	    break;
	case 'g':
	    gkey = optarg;
	    gkey_len = strlen(optarg);
/*  	    gkey = (u_int8_t *)malloc((strlen(optarg)-1)*sizeof(u_int8_t)); */
/*  	    memcpy(gkey,optarg,strlen(optarg)-1); */
/*  	    gkey_len = strlen(optarg)-1; */
	    break;
	case 'u':
	    uid = optarg;
	    break;
	default:
	    fprintf(stderr,"ERROR: unknown option '%c'!\n",c);
	    usage(argv[0]);
	    exit(-1);
	    break;
	}
    }

    argc -= optind;
    argv += optind;
    
    if (argc != 2) {
	fprintf(stderr,
		"ERROR: must supply both clientname and command args!\n");
	exit(-1);
    }
    else {
	client = argv[0];
	command = argv[1];
    }

    if (!secure && (!strcmp(command,"reset") || !strcmp(command,"cycle") 
		   || !strcmp(command,"on") || !strcmp(command,"off"))) {
	fprintf(stderr,"Command '%s' requires secure RMCP.\n",command);
	exit(-3);
    }
    
    rmcp_set_debug_level(debug);
    rmcp_set_enable_warn_err(1);

    ctx = rmcp_ctx_init(timeout,retries);

#ifndef NO_EMULAB
    if (emulab) {
	dbinit();
	/* Attempt lookup in Emulab db */
	if (debug) {
	    fprintf(stderr,"Resolving %s... ",client);
	}
	if (!mydb_nodeidtoip(client,emip)) {
	    fprintf(stderr,"ERROR: could not resolve node_id %s",client);
	    exit(9);
	}
	if (debug) {
	    fprintf(stderr,"%s.\n",emip);
	}

	/* 
	 * If we're in secure mode, grab keys out of the db too if we
	 * need them.
	 */
	if (rkey == NULL) {
	    res = mydb_query("select mykey from outlets_remoteauth"
			     "  where node_id='%s' and key_role='%s'",
			     1,client,role);
	    if (!res) {
		fprintf(stderr,
			"ERROR: could not find keys for %s in Emulab db!\n",
			client);
		exit(10);
	    }
	    
	    nrows = (int)mysql_num_rows(res);
	    if (nrows != 1) {
		fprintf(stderr,"ERROR: could not find role key for Emulab"
			" nodeid %s\n",client);
		exit(11);
	    }

	    row = mysql_fetch_row(res);
	    rkey = (char *)malloc(strlen(row[0])+1);
	    src = row[0];
	    clen = strlen(row[0]) + 1;
	    if (!strncmp("0x",row[0],2)) {
		src = &row[0][2];
		clen -= 2;
	    }
	    strncpy(rkey,src,clen);
	    rkey_len = strlen(rkey);
	    mysql_free_result(res);
	}

	if (gkey == NULL) {
	    res = mydb_query("select mykey from outlets_remoteauth"
                             "  where node_id='%s' and key_role='%s'",
                             1,client,"generation");
            if (!res) {
                fprintf(stderr,
                        "ERROR: could not find gkey for %s in Emulab db!\n",
                        client);
                exit(10);
            }

            nrows = (int)mysql_num_rows(res);
            if (nrows != 1) {
                fprintf(stderr,"ERROR: could not find generation key for"
			" Emulab nodeid %s\n",client);
                exit(11);
            }

            row = mysql_fetch_row(res);
            gkey = (char *)malloc(strlen(row[0])+1);
            src = row[0];
            clen = strlen(row[0]) + 1;
            if (!strncmp("0x",row[0],2)) {
                src = &row[0][2];
                clen -= 2;
            }
            strncpy(gkey,src,clen);
	    gkey_len = strlen(gkey);
	    mysql_free_result(res);
        }

	client = emip;
    }
#endif

    if (secure) {
	if (strcmp(role,"administrator") == 0) {
	    roleno = RMCP_ROLE_ADM;
	}
	else if (strcmp(role,"operator") == 0) {
	    roleno = RMCP_ROLE_OP;
	}
	else {
	    fprintf(stderr,"Unknown role '%s'!\n",role);
	    exit(-4);
	}

	if (hex_mode) {
            char *rkey_tmp = malloc(sizeof(char)*20);
            char *gkey_tmp = malloc(sizeof(char)*20);
            int lpc;
            int outer_lpc = 0;
            for (lpc = 0; lpc < rkey_len; lpc += 2) {
                char cbite[3];
                cbite[2] = '\0';
                if ((lpc + 2) == rkey_len) {
		    strncpy(cbite,&rkey[lpc],1);
		    cbite[1] = '\0';
                }
                else {
		    strncpy(cbite,&rkey[lpc],2);
                }
                int bite = (int)strtol(cbite,NULL,16);
                rkey_tmp[outer_lpc++] = (char)bite;
            }
            rkey_len = outer_lpc;
            outer_lpc = 0;
            for (lpc = 0; lpc < gkey_len; lpc += 2) {
		char cbite[3];
		cbite[2] = '\0';
		if ((lpc + 2) == gkey_len) {
		    strncpy(cbite,&gkey[lpc],1);
		    cbite[1] = '\0';
		}
		else {
		    strncpy(cbite,&gkey[lpc],2);
		}
		int bite = (int)strtol(cbite,NULL,16);
		gkey_tmp[outer_lpc++] = (char)bite;
            }
            gkey_len = outer_lpc;
            gkey = gkey_tmp;
            rkey = rkey_tmp;
        }
	
	rmcp_ctx_setsecure(ctx,
			   roleno,
			   rkey,rkey_len,
			   gkey,gkey_len);
	if (uid) {
	    rmcp_ctx_setuid(ctx,uid,strlen(uid)-1);
	}
    }

    retval = rmcp_open(ctx,client);
    if (retval != RMCP_SUCCESS) {
	fprintf(stderr,
		"Could not open connection to client %s: error %s.\n",
		client,rmcp_error_tostr(retval));
	exit(-5);
    }

    if (secure) {
	retval = rmcp_start_secure_session(ctx);
	if (retval != RMCP_SUCCESS) {
	    fprintf(stderr,
		    "Could not start secure session: error 0x%x\n",
		    retval);
	    exit(-6);
	}
    }

    if (!strcmp(command,COMMAND_PING)) {
	/* send a ping and print the supported protocols */
	retval = rmcp_asf_ping(ctx,&supported);
	if (retval != RMCP_SUCCESS) {
	    fprintf(stderr,
		    "Command '%s' failed: %s\n",
		    command,
		    rmcp_error_tostr(retval));
	    exit(-16);
	}
	rmcp_print_asf_supported(supported,NULL);
    }
    else if (!strcmp(command,COMMAND_CAPABILITIES)) {
	/* get capabilities and print them */
	retval = rmcp_asf_get_capabilities(ctx,&capabilities);
	if (retval != RMCP_SUCCESS) {
	    fprintf(stderr,
		    "Command '%s' failed: %s\n",
		    command,
		    rmcp_error_tostr(retval));
	    exit(-16);
	}
	rmcp_print_asf_capabilities(capabilities,NULL);
    }
    else if (!strcmp(command,COMMAND_STATE)) {
	/* get state and print it */
	retval = rmcp_asf_get_sysstate(ctx,&sysstate);
	if (retval != RMCP_SUCCESS) {
	    fprintf(stderr,
		    "Command '%s' failed: %s\n",
		    command,
		    rmcp_error_tostr(retval));
	    exit(-16);
	}
	rmcp_print_asf_sysstate(sysstate,NULL);
    }
    else if (!strcmp(command,COMMAND_NOP)) {
	/* do nothing */
	fprintf(stdout,
		"As requested, nothing done%s.\n",
		(secure)?" (except secure session establishment)":"");
    }
    else if (!strcmp(command,COMMAND_RESET)) {
	/* reset */
	retval = rmcp_asf_reset(ctx);
	if (retval == RMCP_SUCCESS) {
	    fprintf(stdout,"Reset sent successfully.\n");
	}
	else {
	    fprintf(stderr,"Reset unsuccessful: %s\n",
		    rmcp_error_tostr(retval));
	    exit(-12);
	}
    }
    else if (!strcmp(command,COMMAND_POWER_CYCLE)) {
	/* power cycle */
	retval = rmcp_asf_power_cycle(ctx);
	if (retval == RMCP_SUCCESS) {
	    fprintf(stdout,"Power cycle sent successfully.\n");
	}
	else {
	    fprintf(stderr,"Power cycle unsuccessful: %s\n",
		    rmcp_error_tostr(retval));
	    exit(-12);
	}	
    }
    else if (!strcmp(command,COMMAND_POWER_UP)) {
	/* power up */
	retval = rmcp_asf_power_up(ctx);
	if (retval == RMCP_SUCCESS) {
	    fprintf(stdout,"Power up sent successfully.\n");
	}
	else {
	    fprintf(stderr,"Power up unsuccessful: %s\n",
		    rmcp_error_tostr(retval));
	    exit(-12);
	}	
    }
    else if (!strcmp(command,COMMAND_POWER_DOWN)) {
	/* power down */
	retval = rmcp_asf_power_down(ctx);
	if (retval == RMCP_SUCCESS) {
	    fprintf(stdout,"Power down sent successfully.\n");
	}
	else {
	    fprintf(stderr,"Power down unsuccessful: %s\n",
		    rmcp_error_tostr(retval));
	    exit(-12);
	}	
    }

    retval = rmcp_finalize(ctx);
    if (retval != RMCP_SUCCESS) {
	fprintf(stderr,
		"Could not close session/connection: %s\n",
		rmcp_error_tostr(retval));
	exit(-10);
    }

    exit(0);
}
	
    
    

    
