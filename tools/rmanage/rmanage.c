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
	    "Usage: %s [-hHds] [-tmrkgu <arg>] -c <clientname> <command>\n"
	    "\n"
	    "  -h         Print this message\n"
	    "  -d         Turn on debugging (more d's mean more debug info)\n"
	    "  -s         Use secure RMCP\n"
	    "  -H         Interpret keys as hex strings instead of char strings\n"
	    "  -c host    The hostname of the managed client\n"
	    "  -t timeout Timeout (in seconds) for individual RMCP messages\n"
	    "               (default: %d)\n"
	    "  -m retries Retry N times for unacknowledged RMCP sends\n"
	    "               (default: %d)\n"
	    "  -r role    Use this role (either 'operator' or 'administrator')\n"
	    "  -k key     Use this key with the role specified by '-r'\n"
	    "  -g key     Use this generation key\n"
	    "  -u uid     Send the specified username\n"
	    "\n"
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
    u_int8_t *rkey;
    int rkey_len;
    u_int8_t *gkey;
    int gkey_len;
    int timeout = DEFAULT_TIMEOUT;
    int retries = DEFAULT_RETRIES;
    char *role = NULL;
    int roleno;
    char *uid = NULL;
    char *command = NULL;

    while ((c = getopt(argc,argv,"c:t:m:r:k:g:u:hdsH")) != -1) {
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
	case 'c':
	    client = optarg;
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
    
    if (argc != 1) {
	fprintf(stderr,"ERROR: required command argument is missing!\n");
	exit(-1);
    }
    else {
	command = argv[0];
    }

    if (!secure && (!strcmp(command,"reset") || !strcmp(command,"cycle") 
		   || !strcmp(command,"on") || !strcmp(command,"off"))) {
	fprintf(stderr,"Command '%s' requires secure RMCP.\n",command);
	exit(-3);
    }
    
    rmcp_set_debug_level(debug);
    rmcp_set_enable_warn_err(1);

    ctx = rmcp_ctx_init(timeout,retries);

    if (secure) {
	if (strcmp(role,"administrator") == 0) {
	    roleno = RMCP_ROLE_ADM;
	}
	else if (strcmp(role,"operator") == 0) {
	    roleno = RMCP_ROLE_OP;
	}
	else {
	    fprintf(stderr,"Unknown  role '%s'!\n",role);
	    exit(-4);
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
	
    
    

    
