/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <syslog.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>


/*
 * Directory containing a set of configuration files, one per host
 */
#ifdef USE_CDIR_DB

#define CDIR		"proxydhcp.d"
#define DEFAULTS	"defaults"

struct config {
	struct in_addr	client;
	struct in_addr	server;
	char		bootprog[1];
	/* bootprog is variable length */
};

#define MAX_CONFIGS	1
static int		numconfigs;
static struct config	*parse_configs(char *filename);
static struct config    *default_config;

int
open_db(void)
{
	char	cfile[BUFSIZ];
	
#ifdef TEST
	strcpy(cfile, CDIR);
#else
	strcpy(cfile, CONFPATH);
	strcat(cfile, CDIR);
#endif
	strcat(cfile,"/defaults");
	default_config = parse_configs(cfile);
	return (default_config == NULL);
}

int
query_db(struct in_addr ipaddr, struct in_addr *sip,
	 char *bootprog, int bootlen)
{
	struct config *configp;
	char cfile[BUFSIZ];
	int fd;

	/* Check for a file with this host's name */
#ifdef TEST
	strcpy(cfile,CDIR);
	strcat(cfile,"/");
	strcat(cfile,inet_ntoa(ipaddr));
#else
	strcpy(cfile, CONFPATH);
	strcat(cfile,CDIR);
	strcat(cfile,"/");
	strcat(cfile,inet_nota(ipaddr));
#endif

	configp = parse_configs(cfile);
	if (configp == NULL) {
	    printf("No configuration file %s found, using default\n", cfile);
	    strcpy(bootprog, default_config->bootprog);
	    sip->s_addr = default_config->server.s_addr;
	    return 0;
	}
	
	if (configp->server.s_addr == 0xffffffff) {
#ifdef TEST
	    printf("Using default server\n");
#endif
	    sip->s_addr = default_config->server.s_addr;
	} else {
	    sip->s_addr = configp->server.s_addr;
	}

	if (strcmp(configp->bootprog,"default") == 0) {
#ifdef TEST
	    printf("Using default bootprog\n");
#endif
	    strcpy(bootprog, default_config->bootprog);
	} else {
	    strcpy(bootprog, configp->bootprog);
	}

	return 0;
}

static int
parse_host(char *name)
{
	struct hostent *he;

	if (!isdigit(name[0])) {
	    	if (strcmp(name,"default") == 0) {
		    return 0xffffffff;
		}
		he = gethostbyname(name);
		if (he == 0) {
			syslog(LOG_ERR, "%s: unknown host", name);
			return 0;
		}
		return *(int *)he->h_addr;
	}
	return inet_addr(name);
}

static struct config*
parse_configs(char *filename)
{
	FILE	*fp;
	int	i;
	char	buf[BUFSIZ], *bp, *cix;
	char	client[256], server[256], bootprog[256];
	struct  config *configp;
	int	ipaddr;

	if ((fp = fopen(filename, "r")) == NULL) {
		syslog(LOG_ERR, "%s: cannot open", filename);
		return NULL;
	}

	while (1) {
		if ((bp = fgets(buf, BUFSIZ, fp)) == NULL)
			break;

		if (*bp == '\n' || *bp == '#')
			continue;

		if (numconfigs >= MAX_CONFIGS) {
			syslog(LOG_ERR, "%s: too many lines", filename);
			fclose(fp);
			return NULL;
		}

		sscanf(bp, "%s %s\n", &server, &bootprog);

		configp = (struct config *)
			calloc(sizeof(*configp) + strlen(bootprog), 1);
		if (!configp) {
		bad:
			syslog(LOG_ERR, "%s: parse error", filename);
			fclose(fp);
			return NULL;
		}
		
		strcpy(configp->bootprog, bootprog);
		
		if ((ipaddr = parse_host(server)) == 0)
			goto bad;
		configp->server.s_addr = ipaddr;
	}
	fclose(fp);
	return configp;
}

#ifdef TEST
#include <stdarg.h>

void
syslog(int prio, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

main(int argc, char **argv)
{
	struct in_addr ipaddr, server;
	char	       bootprog[1024];

	if (open_db() != 0) {
		printf("bad config file\n");
		exit(1);
	}

	while (--argc > 0) {
		if (ipaddr.s_addr = parse_host(*++argv)) {
			if (query_db(ipaddr, &server, bootprog, 1024) == 0) {
				printf("%s: %s %s\n", *argv,
				       inet_ntoa(server), bootprog);
			} else
				printf("failed\n");
		} else
			printf("bogus IP address `%s'\n", *argv);
	}
	exit(0);
}
#endif
#endif
