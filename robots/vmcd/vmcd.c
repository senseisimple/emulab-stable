
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "mtp.h"

static int debug = 0;

static int looping = 1;

static char *emc_hostname = NULL;
static int emc_port = 0;

static char *logfile = NULL;

static char *pidfile = NULL;

struct vmc_client {
    char *vc_hostname;
    int vc_port;
    int vc_fd;
};

#define MAX_VMC_CLIENTS 128

static unsigned int vmc_client_count = 0;
static struct vmc_client vmc_clients[MAX_VMC_CLIENTS];

static struct mtp_config_vmc *vmc_config = NULL;

static void sigquit(int signal)
{
    looping = 0;
}

static int mygethostbyname(struct sockaddr_in *host_addr,
			   char *host,
			   int port)
{
    struct hostent *host_ent;
    int retval = 0;

    assert(host_addr != NULL);
    assert(host != NULL);
    assert(strlen(host) > 0);

    memset(host_addr, 0, sizeof(struct sockaddr_in));
    host_addr->sin_len = sizeof(struct sockaddr_in);
    host_addr->sin_family = AF_INET;
    host_addr->sin_port = htons(port);
    if( (host_ent = gethostbyname(host)) != NULL ) {
	memcpy((char *)&host_addr->sin_addr.s_addr,
	       host_ent->h_addr,
	       host_ent->h_length);
	retval = 1;
    }
    else {
	retval = inet_aton(host, &host_addr->sin_addr);
    }
    return( retval );
}

static void usage(void)
{
    fprintf(stderr,
	    "Usage: vmcd [-hd] [-l logfile] [-e emchost] [-p emcport]\n"
	    "            [-c clienthost] [-P clientport]\n"
	    "Vision master control daemon\n"
	    "  -h\tPrint this message\n"
	    "  -d\tTurn on debugging messages and do not daemonize\n"
	    "  -l logfile\tSpecify the log file to use\n"
	    "  -i pidfile\tSpecify the pid file name\n"
	    "  -e emchost\tSpecify the host name where emc is running\n"
	    "  -p emcport\tSpecify the port that emc is listening on\n"
	    "  -c clienthost\tSpecify the host name where a vmc-client is\n"
	    "  -P clientport\tSpecify the port the client is listening on\n"
	    "\n"
	    "Example:\n"
	    "  $ vmcd -c foo -P 7070 -- -c foo -P 7071\n");
}

static int parse_client_options(int *argcp, char **argvp[])
{
    int c, argc, retval = EXIT_SUCCESS;
    char **argv;

    assert(argcp != NULL);
    assert(argvp != NULL);

    argc = *argcp;
    argv = *argvp;
    
    while ((c = getopt(argc, argv, "hdp:l:i:e:c:P:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'l':
	    logfile = optarg;
	    break;
	case 'i':
	    pidfile = optarg;
	    break;
	case 'e':
	    emc_hostname = optarg;
	    break;
	case 'p':
	    if (sscanf(optarg, "%d", &emc_port) != 1) {
		error("-p option is not a number: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'c':
	    vmc_clients[vmc_client_count].vc_hostname = optarg;
	    break;
	case 'P':
	    if (sscanf(optarg,
		       "%d",
		       &vmc_clients[vmc_client_count].vc_port) != 1) {
		error("-P option is not a number: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	default:
	    break;
	}
    }

    if (vmc_clients[vmc_client_count].vc_port != 0) {
	vmc_client_count += 1;
    }

    argc -= optind;
    argv += optind;

    *argcp = argc + 1;
    *argvp = argv - 1;

    optreset = 1;
    optind = 1;

    return retval;
}

int main(int argc, char *argv[])
{
    int emc_fd = 0, lpc, retval = EXIT_SUCCESS;
    struct sockaddr_in sin;
    fd_set readfds;

    FD_ZERO(&readfds);
    
    do {
	retval = parse_client_options(&argc, &argv);
    } while ((argc > 0) && strcmp(argv[0], "--") == 0);

    if (debug) {
	loginit(0, logfile);
    }
    else {
	/* Become a daemon */
	daemon(0, 0);
	
	if (logfile)
	    loginit(0, logfile);
	else
	    loginit(1, "vmcclient");
    }

    if (pidfile) {
	FILE *fp;

	if ((fp = fopen(pidfile, "w")) != NULL) {
	    fprintf(fp, "%d\n", getpid());
	    (void) fclose(fp);
	}
    }

    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);

    if (emc_hostname != NULL) {
	struct mtp_packet *mp = NULL, *rmp = NULL;
	struct mtp_control mc;

	mc.id = -1;
	mc.code = -1;
	mc.msg = "vmcd init";
	if (mygethostbyname(&sin, emc_hostname, emc_port) == 0) {
	    pfatal("bad emc hostname: %s\n", emc_hostname);
	}
	else if ((emc_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    pfatal("socket");
	}
	else if (connect(emc_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    pfatal("connect");
	}
	else if ((mp = mtp_make_packet(MTP_CONTROL_INIT,
				       MTP_ROLE_VMC,
				       &mc)) == NULL) {
	    pfatal("mtp_make_packet");
	}
	else if (mtp_send_packet(emc_fd, mp) != MTP_PP_SUCCESS) {
	    pfatal("could not configure with emc");
	}
	else if (mtp_receive_packet(emc_fd, &rmp) != MTP_PP_SUCCESS) {
	    pfatal("could not get vmc config packet");
	}
	else {
	    FD_SET(emc_fd, &readfds);
	    vmc_config = rmp->data.config_vmc;

	    for (lpc = 0; lpc < vmc_config->num_robots; lpc++) {
		info(" robot[%d] = %d %s\n",
		     lpc,
		     vmc_config->robots[lpc].id,
		     vmc_config->robots[lpc].hostname);
	    }
	}

	free(mp);
	mp = NULL;
    }
    
    for (lpc = 0; lpc < vmc_client_count; lpc++) {
	struct vmc_client *vc = &vmc_clients[lpc];
	
	if (mygethostbyname(&sin, vc->vc_hostname, vc->vc_port) == 0) {
	    pfatal("bad vmc-client hostname: %s\n", vc->vc_hostname);
	}
	else if ((vc->vc_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    pfatal("socket");
	}
	else if (connect(vc->vc_fd,
			 (struct sockaddr *)&sin,
			 sizeof(sin)) == -1) {
	    pfatal("connect");
	}
	else {
	    FD_SET(vc->vc_fd, &readfds);
	}
    }

    while (looping) {
	fd_set rreadyfds;
	int rc;

	rreadyfds = readfds;
	rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);
	if (rc > 0) {
	    if ((emc_fd != 0) && FD_ISSET(emc_fd, &rreadyfds)) {
		struct mtp_packet *mp = NULL;
		
		if ((rc = mtp_receive_packet(emc_fd, &mp)) != MTP_PP_SUCCESS) {
		    fatal("bad packet from emc %d\n", rc);
		}
		else {
		    /* XXX Need to handle update_id msgs here. */
		    mtp_print_packet(stdout, mp);
		}

		mtp_free_packet(mp);
		mp = NULL;
	    }
	    
	    for (lpc = 0; lpc < vmc_client_count; lpc++) {
		struct vmc_client *vc = &vmc_clients[lpc];

		if (FD_ISSET(vc->vc_fd, &rreadyfds)) {
		    struct mtp_packet *mp = NULL;
		    
		    if (mtp_receive_packet(vc->vc_fd, &mp) != MTP_PP_SUCCESS) {
			fatal("bad packet from vmc-client\n");
		    }
		    else {
			/* XXX Need to handle update_position msgs here. */
			mtp_print_packet(stdout, mp);
		    }
		    
		    mtp_free_packet(mp);
		    mp = NULL;
		}
	    }
	}
	else if (rc == -1) {
	    perror("select");
	}
    }
    
    return retval;
}
