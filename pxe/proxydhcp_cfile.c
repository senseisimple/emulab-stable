/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <syslog.h>

/*
 * Trivial config file format.
 */
#ifdef USE_CFILE_DB

#define CFILE	"proxydhcp.conf"

struct config {
	struct in_addr	client;
	struct in_addr	server;
	char		bootprog[1];
	/* bootprog is variable length */
};

#define MAX_CONFIGS	256
static int		numconfigs;
static struct config	*configurations[MAX_CONFIGS];
static struct config	*find_config(struct in_addr addr);
static int		parse_configs(char *filename);

int
open_db(void)
{
	char	cfile[BUFSIZ];
	
#ifdef TEST
	strcpy(cfile, CFILE);
#else
	strcpy(cfile, CONFPATH);
	strcat(cfile, CFILE);
#endif
	return parse_configs(cfile);
}

int
query_db(struct in_addr ipaddr, struct in_addr *sip,
	 char *bootprog, int bootlen)
{
	struct config *configp;

	if ((configp = find_config(ipaddr)) == 0)
		return 1;

	if (strlen(configp->bootprog) >= bootlen)
		return 1;

	sip->s_addr = configp->server.s_addr;
	strcpy(bootprog, configp->bootprog);
	
	return 0;
}

int
close_db(void)
{
	int i;

	for (i = 0; i < numconfigs; i++)
		if (configurations[i])
			free(configurations[i]);
	numconfigs = 0;
	return 0;
}

static int
parse_host(char *name)
{
	struct hostent *he;

	if (!isdigit(name[0])) {
		he = gethostbyname(name);
		if (he == 0) {
			syslog(LOG_ERR, "%s: unknown host", name);
			return 0;
		}
		return *(int *)he->h_addr;
	}
	return inet_addr(name);
}

static int
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
		return 1;
	}

	while (1) {
		if ((bp = fgets(buf, BUFSIZ, fp)) == NULL)
			break;

		if (*bp == '\n' || *bp == '#')
			continue;

		if (numconfigs >= MAX_CONFIGS) {
			syslog(LOG_ERR, "%s: too many lines", filename);
			fclose(fp);
			return 1;
		}

		sscanf(bp, "%s %s %s\n", &client, &server, &bootprog);

		configp = (struct config *)
			calloc(sizeof(*configp) + strlen(bootprog), 1);
		if (!configp) {
		bad:
			syslog(LOG_ERR, "%s: parse error", filename);
			fclose(fp);
			close_db();
			return 1;
		}
		configurations[numconfigs++] = configp;
		
		strcpy(configp->bootprog, bootprog);
		
		if ((ipaddr = parse_host(client)) == 0)
			goto bad;
		configp->client.s_addr = ipaddr;
		
		if ((ipaddr = parse_host(server)) == 0)
			goto bad;
		configp->server.s_addr = ipaddr;
	}
	fclose(fp);
	return 0;
}

static struct config *
find_config(struct in_addr addr)
{
	int		i;
	struct config	*configp;
	
	for (i = 0; i < numconfigs; i++)
		if ((configp = configurations[i]) != NULL &&
		    addr.s_addr == configp->client.s_addr)
			return configp;

	return 0;
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
	char	       *bootprog;

	if (open_db() != 0) {
		printf("bad config file\n");
		exit(1);
	}

	while (--argc > 0) {
		if (ipaddr.s_addr = parse_host(*++argv)) {
			if (query_db(ipaddr, &server, &bootprog) == 0) {
				printf("%s: %s %s\n", *argv,
				       inet_ntoa(server), bootprog);
			} else
				printf("failed\n");
		} else
			printf("bogus IP address `%s'\n", *argv);
	}
	close_db();
	exit(0);
}
#endif
#endif
