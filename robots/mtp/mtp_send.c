
#include "config.h"

#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "mtp.h"

static int debug = 0;

static int mygethostbyname(struct sockaddr_in *host_addr, char *host)
{
    struct hostent *host_ent;
    int retval = 0;

    assert(host_addr != NULL);
    assert(host != NULL);
    assert(strlen(host) > 0);

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
	    "Usage: mtp_send [OPTIONS] <command>\n"
	    "Commands:\n"
	    "  error notify init close request-position request-id\n"
	    "  update-position update-id command-goto command-stop\n"
	    "\n"
	    "Options:\n"
	    "  -h\t\tPrint this message\n"
	    "  -d\t\tTurn on debugging messages\n"
	    "  -w\t\tWait for, and print, any replies\n"
	    "  -i #\t\tThe robot identifier\n"
	    "  -c #\t\tThe code number for control packets\n"
	    "  -C #\t\tThe command_id for goto/stop\n"
	    "  -n name\tThe robot name\n"
	    "  -x #\t\tThe X coordinate\n"
	    "  -y #\t\tThe Y coordinate\n"
	    "  -o #\t\tThe orientation\n"
	    "  -H #\t\tThe horizontal size\n"
	    "  -V #\t\tThe vertical size\n"
	    "  -t #\t\tThe timestamp\n"
	    "  -s idle|moving|error|complete\n"
	    "      \t\tThe robot status\n"
	    "  -m msg\tThe msg for control packets\n"
	    "  -r vmc|emc|rmc|emulab\n"
	    "      \t\tThe role (Default: rmc)\n"
	    "  -P #\t\tThe port the peer listening on\n"
	    "\n"
	    "Example:\n"
	    "  $ mtp_send -n garcia1 -P 3000 -i 1 -c 2 -m \"FooBar\" error\n");
}

static void required_option(char *name)
{
    assert(name != NULL);
    
    fprintf(stderr,
	    "error: the -%s option is required for this packet type\n",
	    name);
    usage();
    exit(1);
}

static int interpret_options(int *argcp, char ***argvp)
{
    static int fd = -1;
    
    static struct {
	int id;
	int code;
	char *hostname;
	int command_id;
	float x;
	float y;
	float orientation;
	float horizontal;
	float vertical;
	int timestamp;
	int status;
	char *message;
	int role;

	int port;
    } args = {
	-1,		/* id */
	-1,		/* code */
	NULL,		/* hostname */
	-1,		/* command_id */
	DBL_MIN,	/* x */
	DBL_MIN,	/* y */
	DBL_MIN,	/* orientation */
	DBL_MIN,	/* horizontal */
	DBL_MIN,	/* vertical */
	-1,		/* timestamp */
	-1,		/* status */
	NULL,		/* message */
	MTP_ROLE_RMC,	/* role */
	-1,		/* port */
    };

    int argc = *argcp;
    char **argv = *argvp;

    union {
	struct mtp_control control;
	struct mtp_config_rmc config_rmc;
	struct mtp_config_vmc config_vmc;
	struct mtp_request_position request_position;
	struct mtp_request_id request_id;
	struct mtp_update_position update_position;
	struct mtp_update_id update_id;
	struct mtp_command_goto command_goto;
	struct mtp_command_stop command_stop;
    } payload;
    
    int c, waitmode = 0, retval = EXIT_SUCCESS;
    mtp_payload_t m_payload;
    struct sockaddr_in sin;
    mtp_packet_t *mp;
    char opcode;
    
    while ((c = getopt(argc, argv, "hdwi:c:C:n:x:y:o:H:V:t:s:m:r:P:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'w':
	    waitmode = 1;
	    break;
	case 'i':
	    if (sscanf(optarg, "%d", &args.id) != 1) {
		fprintf(stderr,
			"error: i option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'c':
	    if (sscanf(optarg, "%d", &args.code) != 1) {
		fprintf(stderr,
			"error: c option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'C':
	    if (sscanf(optarg, "%d", &args.command_id) != 1) {
		fprintf(stderr,
			"error: C option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'n':
	    if (strlen(optarg) == 0) {
		fprintf(stderr, "error: n option is empty\n");
		usage();
		exit(1);
	    }
	    args.hostname = optarg;
	    break;
	case 'x':
	    if (sscanf(optarg, "%f", &args.x) != 1) {
		fprintf(stderr,
			"error: x option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'y':
	    if (sscanf(optarg, "%f", &args.y) != 1) {
		fprintf(stderr,
			"error: y option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'o':
	    if (sscanf(optarg, "%f", &args.orientation) != 1) {
		fprintf(stderr,
			"error: o option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'H':
	    if (sscanf(optarg, "%f", &args.horizontal) != 1) {
		fprintf(stderr,
			"error: H option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'V':
	    if (sscanf(optarg, "%f", &args.vertical) != 1) {
		fprintf(stderr,
			"error: V option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 't':
	    if (sscanf(optarg, "%d", &args.timestamp) != 1) {
		fprintf(stderr,
			"error: t option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 's':
	    if (strcasecmp(optarg, "idle") == 0) {
		args.status = MTP_POSITION_STATUS_IDLE;
	    }
	    else if (strcasecmp(optarg, "moving") == 0) {
		args.status = MTP_POSITION_STATUS_MOVING;
	    }
	    else if (strcasecmp(optarg, "error") == 0) {
		args.status = MTP_POSITION_STATUS_ERROR;
	    }
	    else if (strcasecmp(optarg, "complete") == 0) {
		args.status = MTP_POSITION_STATUS_COMPLETE;
	    }
	    else {
		fprintf(stderr,
			"error: s option must be one of: idle, moving, "
			"error, or complete\n");
		usage();
		exit(1);
	    }
	    break;
	case 'm':
	    args.message = optarg;
	    break;
	case 'r':
	    if (strcasecmp(optarg, "vmc") == 0) {
		args.role = MTP_ROLE_VMC;
	    }
	    else if (strcasecmp(optarg, "emc") == 0) {
		args.role = MTP_ROLE_EMC;
	    }
	    else if (strcasecmp(optarg, "rmc") == 0) {
		args.role = MTP_ROLE_RMC;
	    }
	    else if (strcasecmp(optarg, "emulab") == 0) {
		args.role = MTP_ROLE_EMULAB;
	    }
	    else {
		fprintf(stderr,
			"error: r option must be one of: vmc, emc, or rmc\n");
		usage();
		exit(1);
	    }
	    break;
	case 'P':
	    if (sscanf(optarg, "%d", &args.port) != 1) {
		fprintf(stderr,
			"error: t option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	default:
	    usage();
	    exit(1);
	    break;
	}
    }

    argc -= optind;
    argv += optind;

    if (argc == 0) {
	fprintf(stderr, "error: missing command name argument\n");
	usage();
	exit(1);
    }

    *argcp = argc - 1;
    *argvp = argv + 1;

    if (args.hostname == NULL) {
	required_option("n");
    }
    if (args.port == -1) {
	required_option("P");
    }
    
    if (strcasecmp(argv[0], "error") == 0)
	opcode = MTP_CONTROL_ERROR;
    else if (strcasecmp(argv[0], "notify") == 0)
	opcode = MTP_CONTROL_NOTIFY;
    else if (strcasecmp(argv[0], "init") == 0)
	opcode = MTP_CONTROL_INIT;
    else if (strcasecmp(argv[0], "close") == 0)
	opcode = MTP_CONTROL_CLOSE;
    else if (strcasecmp(argv[0], "config-vmc") == 0)
	opcode = MTP_CONFIG_VMC;
    else if (strcasecmp(argv[0], "config-rmc") == 0)
	opcode = MTP_CONFIG_RMC;
    else if (strcasecmp(argv[0], "request-position") == 0)
	opcode = MTP_REQUEST_POSITION;
    else if (strcasecmp(argv[0], "request-id") == 0)
	opcode = MTP_REQUEST_ID;
    else if (strcasecmp(argv[0], "update-position") == 0)
	opcode = MTP_UPDATE_POSITION;
    else if (strcasecmp(argv[0], "update-id") == 0)
	opcode = MTP_UPDATE_ID;
    else if (strcasecmp(argv[0], "command-goto") == 0)
	opcode = MTP_COMMAND_GOTO;
    else if (strcasecmp(argv[0], "command-stop") == 0)
	opcode = MTP_COMMAND_STOP;
    else {
	fprintf(stderr, "error: unknown command: %s\n", argv[0]);
	usage();
	exit(1);
    }

    if (debug > 0) {
	fprintf(stderr,
		"Opcode: %d\n"
		"  id:\t\t%d\n"
		"  code:\t\t%d\n"
		"  hostname:\t%s\n"
		"  x:\t\t%f\n"
		"  y:\t\t%f\n"
		"  orientation:\t%f\n"
		"  horizontal:\t%f\n"
		"  vertical:\t%f\n"
		"  timestamp:\t%d\n"
		"  status:\t%d\n"
		"  message:\t%s\n"
		"  port:\t\t%d\n",
		opcode,
		args.id,
		args.code,
		args.hostname,
		args.x,
		args.y,
		args.orientation,
		args.horizontal,
		args.vertical,
		args.timestamp,
		args.status,
		args.message,
		args.port);
    }

    switch (opcode) {
	
    case MTP_CONTROL_ERROR:
    case MTP_CONTROL_NOTIFY:
    case MTP_CONTROL_INIT:
    case MTP_CONTROL_CLOSE:
	if (args.id == -1)
	    required_option("i");
	if (args.code == -1)
	    required_option("c");
	if (args.message == NULL)
	    required_option("m");
	
	payload.control.id = args.id;
	payload.control.code = args.code;
	payload.control.msg = args.message;

	m_payload.control = &payload.control;
	break;

    case MTP_CONFIG_VMC:
    case MTP_CONFIG_RMC:
	fprintf(stderr, "internal error: not supported at the moment...\n");
	assert(0);
	break;

    case MTP_REQUEST_POSITION:
	if (args.id == -1)
	    required_option("i");

	payload.request_position.robot_id = args.id;

	m_payload.request_position = &payload.request_position;
	break;

    case MTP_REQUEST_ID:
	if (args.x == DBL_MIN)
	    required_option("x");
	if (args.y == DBL_MIN)
	    required_option("y");
	if (args.orientation == DBL_MIN)
	    required_option("o");
	if (args.timestamp == -1)
	    required_option("t");

	payload.request_id.position.x = args.x;
	payload.request_id.position.y = args.y;
	payload.request_id.position.theta = args.orientation;
	payload.request_id.position.timestamp = args.timestamp;

	m_payload.request_id = &payload.request_id;
	break;

    case MTP_UPDATE_POSITION:
	if (args.id == -1)
	    required_option("i");
	if (args.x == DBL_MIN)
	    required_option("x");
	if (args.y == DBL_MIN)
	    required_option("y");
	if (args.orientation == DBL_MIN)
	    required_option("o");
	if (args.status == -1)
	    required_option("s");
	if (args.timestamp == -1)
	    required_option("t");

	payload.update_position.robot_id = args.id;
	payload.update_position.position.x = args.x;
	payload.update_position.position.y = args.y;
	payload.update_position.position.theta = args.orientation;
	payload.update_position.position.timestamp = args.timestamp;
	payload.update_position.status = args.status;

	m_payload.update_position = &payload.update_position;
	break;

    case MTP_UPDATE_ID:
	if (args.id == -1)
	    required_option("i");

	payload.update_id.robot_id = args.id;

	m_payload.update_id = &payload.update_id;
	break;

    case MTP_COMMAND_GOTO:
	if (args.id == -1)
	    required_option("i");
	if (args.x == DBL_MIN)
	    required_option("x");
	if (args.y == DBL_MIN)
	    required_option("y");
	if (args.orientation == DBL_MIN)
	    required_option("o");

	payload.command_goto.command_id = args.command_id;
	payload.command_goto.robot_id = args.id;
	payload.command_goto.position.x = args.x;
	payload.command_goto.position.y = args.y;
	payload.command_goto.position.theta = args.orientation;

	m_payload.command_goto = &payload.command_goto;
	break;

    case MTP_COMMAND_STOP:
	if (args.id == -1)
	    required_option("i");

	payload.command_stop.command_id = args.command_id;
	payload.command_stop.robot_id = args.id;

	m_payload.command_stop = &payload.command_stop;
	break;

    default:
	fprintf(stderr,
		"internal error: command %s not supported at the moment...\n",
		argv[0]);
	assert(0);
	break;
    }

    if (fd == -1) {
	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(args.port);
	
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket");
	}
	else if (mygethostbyname(&sin, args.hostname) == 0) {
	    perror("gethostbyname");
	}
	else if (connect(fd,
			 (struct sockaddr *)&sin,
			 sizeof(struct sockaddr_in)) == -1) {
	    perror("connect");
	}
    }

    if (fd == -1) {
	fprintf(stderr, "could not connect to server\n");
	exit(1);
    }
    else if ((mp = mtp_make_packet(opcode,
				   args.role,
				   m_payload.control)) == NULL) {
	perror("mtp_make_packet");
    }
    else if (mtp_send_packet(fd, mp) != MTP_PP_SUCCESS) {
	perror("mtp_send_packet"); // XXX not right
    }
    else {
	if (debug) {
	    mtp_print_packet(stdout, mp);
	}
	if (waitmode) {
	    if (mtp_receive_packet(fd, &mp) != MTP_PP_SUCCESS) {
		perror("mtp_receive_packet");
	    }
	    else {
		mtp_print_packet(stdout, mp);

		mtp_free_packet(mp);
		mp = NULL;
	    }
	}
    }

    optreset = 1;
    optind = 1;
    
    return retval;
}

int main(int argc, char *argv[])
{
    int retval;

    do {
	retval = interpret_options(&argc, &argv);
    } while ((argc > 0) && strcmp(argv[0], "--") == 0);

    return retval;
}
