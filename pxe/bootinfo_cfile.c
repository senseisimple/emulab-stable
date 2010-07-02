/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <syslog.h>
#include "log.h"
#include "bootwhat.h"
#include "bootinfo.h"

/*
 * Trivial config file format.
 * Swiped from proxydhcp.c.
 */
#ifdef USE_CFILE_DB

#define CFILE	"bootinfo.conf"

struct config {
	struct in_addr	client;
	int		bootinfolen;
	boot_what_t	bootinfo;
	/* bootinfo is variable length */
};

#define MAX_CONFIGS	256
static int		numconfigs;
static struct config	*configurations[MAX_CONFIGS];
static struct config	*find_config(struct in_addr addr);
static int		parse_configs(char *filename);
#ifdef TEST
static void		print_bootwhat(boot_what_t *bootinfo);
#endif

int
open_bootinfo_db(void)
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
query_bootinfo_db(struct in_addr ipaddr, int version, boot_what_t *info, char *key)
{
	struct config *configp;

	if ((configp = find_config(ipaddr)) == 0)
		return 1;

	memcpy(info, &configp->bootinfo, configp->bootinfolen);
	return 0;
}

int
close_bootinfo_db(void)
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
			error("%s: unknown host", name);
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
	char	buf[BUFSIZ], *bp, *cix, client[256], action[256];
	struct  config *configp;
	int	ipaddr;

	if ((fp = fopen(filename, "r")) == NULL) {
		error("%s: cannot open", filename);
		return 1;
	}

	while (1) {
		if ((bp = fgets(buf, BUFSIZ, fp)) == NULL)
			break;

		if (*bp == '\n' || *bp == '#')
			continue;

		if (numconfigs >= MAX_CONFIGS) {
			error("%s: too many lines", filename);
			fclose(fp);
			return 1;
		}

		sscanf(bp, "%s %s\n", client, action);

		configp = (struct config *) calloc(sizeof *configp, 1);
		if (!configp) {
		bad:
			error("%s: parse error", filename);
			fclose(fp);
			close_bootinfo_db();
			return 1;
		}
		configurations[numconfigs++] = configp;
		if ((ipaddr = parse_host(client)) == 0)
			goto bad;

		configp->client.s_addr = ipaddr;
		configp->bootinfolen = sizeof *configp;
		configp->bootinfo.flags = 0;
		if (strncmp(action, "sysid=", 6) == 0) {
			configp->bootinfo.type = BIBOOTWHAT_TYPE_SYSID;
			configp->bootinfo.what.sysid = atoi(&action[6]);
		} else if (strncmp(action, "part=", 5) == 0) {
			configp->bootinfo.type = BIBOOTWHAT_TYPE_PART;
			configp->bootinfo.what.partition = atoi(&action[5]);
		} else if (strncmp(action, "file=", 5) == 0) {
			if ((cix = index(action, ':')) == 0)
				goto bad;
			configp->bootinfo.type = BIBOOTWHAT_TYPE_MB;
			*cix++ = 0;
			if ((ipaddr = parse_host(&action[5])) == 0)
				goto bad;
			configp->bootinfo.what.mb.tftp_ip.s_addr = ipaddr;
			configp = realloc(configp,
					  sizeof *configp + strlen(cix) + 1);
			if (configp == 0)
				goto bad;
				
			configurations[numconfigs-1] = configp;
			configp->bootinfolen += strlen(cix) + 1;
			strcpy(configp->bootinfo.what.mb.filename, cix);
		} else
			goto bad;
	}
	fclose(fp);

#ifdef TEST
	printf("==== %s ====\n", filename);
	for (i = 0; i < numconfigs; i++) {
		configp = configurations[i];
		printf("%s: ", inet_ntoa(configp->client));
		print_bootwhat(&configp->bootinfo);
	}
	printf("====\n\n");
#endif

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

static void
print_bootwhat(boot_what_t *bootinfo)
{
	switch (bootinfo->type) {
	case BIBOOTWHAT_TYPE_PART:
		printf("boot from partition %d\n",
		       bootinfo->what.partition);
		break;
	case BIBOOTWHAT_TYPE_SYSID:
		printf("boot from partition with sysid %d\n",
		       bootinfo->what.sysid);
		break;
	case BIBOOTWHAT_TYPE_MB:
		printf("boot multiboot image %s:%s\n",
		       inet_ntoa(bootinfo->what.mb.tftp_ip),
		       bootinfo->what.mb.filename);
		break;
	}
}

main(int argc, char **argv)
{
	struct in_addr ipaddr;
	boot_info_t boot_info;
	boot_what_t *boot_whatp = (boot_what_t *)&boot_info.data;

	if (open_bootinfo_db() != 0) {
		printf("bad config file\n");
		exit(1);
	}

	while (--argc > 0) {
		if (inet_aton(*++argv, &ipaddr)) {
			if (query_bootinfo_db(ipaddr, boot_whatp) == 0) {
				printf("%s: ", *argv);
				print_bootwhat(boot_whatp);
			} else
				printf("failed\n");
		} else
			printf("bogus IP address `%s'\n", *argv);
	}
	close_bootinfo_db();
	exit(0);
}
#endif
#endif
