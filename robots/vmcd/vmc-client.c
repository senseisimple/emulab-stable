/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "log.h"
#include "mtp.h"
#include <math.h>

#if defined(HAVE_MEZZANINE)
#  include "mezz.h"
#else

#  warning "Mezzanine not available, simulating instead."

#  define MEZZ_MAX_OBJECTS 100

typedef struct {
    int class[2];       // Color class for the two blobs
    int valid;
    double max_disp;    // Maximum inter-frame displacement
    double max_sep;     // Maximum blob separation
    int max_missed;     // Max frames before looking for a new match
    int missed;         // Number of missed frames (object not seen)
    double px, py, pa;  // Object pose (world cs).
} mezz_object_t;

typedef struct {
    int count;
    mezz_object_t objects[MEZZ_MAX_OBJECTS];
} mezz_objectlist_t;

typedef struct {
    double time;
    mezz_objectlist_t objectlist;
} mezz_mmap_t;

/**
 * File to read simulated positions from.
 */
static FILE *posfile = NULL;

#endif

/**
 * Default port to listen for client connections.
 */
#define VMCCLIENT_DEFAULT_PORT 6969

/**
 * Debugging flag.
 */
static int debug = 0;

/**
 * Flag that indicates whether or not to continue in the main loop.
 */
static volatile int looping = 1;

/**
 * Counter used to track when a new frame was received from mezzanine.
 */
static volatile unsigned int mezz_frame_count = 0;

/**
 * X offset from the real world X == 0 to our local camera X == 0.
 */
static double x_offset = 0.0;

/**
 * Y offset from the real world Y == 0 to our local camera Y == 0.
 */
static double y_offset = 0.0;

static double z_offset = 0.0;

static XDR xdr;
static char packet_buffer[2048];
static char *cursor;

/**
 * Print the usage statement to standard error.
 */
static void usage(void)
{
    fprintf(stderr,
            "Usage: vmc-client [-hd] [-p port] [-x off] [-y off] mezzfile\n"
            "Required arguments:\n"
            "  mezzfile\tThe name of the mezzanine shared file\n"
            "Options:\n"
            "  -h\t\tPrint this message\n"
            "  -d\t\tTurn on debugging messages and do not daemonize\n"
            "  -l logfile\tSpecify the log file\n"
            "  -p port\tSpecify the port number to listen on. (Default: %d)\n"
            "  -x offset\tx offset from real world x = 0 to our local x = 0\n"
            "  -y offset\ty offset from real world y = 0 to our local y = 0\n"
#if !defined(HAVE_MEZZANINE)
	    "  -f file\tFile to read simulated positions from.\n"
#endif
	    ,
            VMCCLIENT_DEFAULT_PORT);
}

static int mem_write(char *handle, char *data, int len)
{
    int retval = len;

    assert(handle == NULL);
    assert(data != NULL);

    memcpy(cursor, data, len);
    cursor += len;
    
    return retval;
}

/**
 * Signal handler for SIGINT, SIGQUIT, and SIGTERM that just stops the main
 * loop.
 *
 * @param signal The actual signal received.
 */
static void sigquit(int signal)
{
    assert((signal == SIGINT) || (signal == SIGQUIT) || (signal == SIGTERM));
    
    looping = 0;
}

/**
 * Signal handler for SIGUSR1 that updates the mezzanine frame count so the
 * main loop knows there is a new frame available.
 *
 * @param signal The actual signal received.
 */
static void sigusr1(int signal)
{
    assert(signal == SIGUSR1);
    
    mezz_frame_count += 1;
}

#define ROBOT_HEIGHT 0.12f

void radial_trans(struct robot_position *p_inout)
{
    float distance_from_center, theta, vtheta, offset;
    struct robot_position rp;
    
    rp = *p_inout;

    mtp_polar(NULL, &rp, &distance_from_center, &theta);
    vtheta = atan2f(z_offset, distance_from_center);
    offset = ROBOT_HEIGHT / tanf(vtheta);
    mtp_cartesian(NULL, distance_from_center - offset, theta, p_inout);
}

/**
 * Transform a local camera coordinate to a real world coordinate.
 *
 * @param p The position to transform.
 */
void local2global_posit_trans(struct robot_position *p_inout)
{
    float old_x = p_inout->x;
    
    assert(p_inout != NULL);
    
    p_inout->x = p_inout->y + x_offset;
    p_inout->y = old_x + y_offset;
    p_inout->theta -= M_PI_2;
}

/**
 * Encode any object identified by mezzanine as mtp packets in the given
 * buffer.
 *
 * @param mm The mezzanine memory-mapped file.
 * @return The length, in bytes, of the packets encoded in the buffer.
 */
static int encode_packets(mezz_mmap_t *mm)
{
    struct mtp_update_position *mup;
    mezz_objectlist_t *mol;
    struct mtp_packet mp;
    int lpc, retval = 0;
    int last_idx_set;
    
    assert(mm != NULL);

    mol = &mm->objectlist;

    cursor = packet_buffer;
    
#if !defined(HAVE_MEZZANINE)
    if (posfile) {
	if (fscanf(posfile, "%d", &mol->count) != 1) {
	    error("bad position file, expecting object count\n");
	}
	else {
	    for (lpc = 0; lpc < mol->count; lpc++) {
		if (fscanf(posfile,
			   "%lf %lf %lf",
			   &mol->objects[lpc].px,
			   &mol->objects[lpc].py,
			   &mol->objects[lpc].pa) != 3) {
		    error("bad position file, expecting coords\n");
		}
	    }
	}
    }
#endif

    /*
     * Initialize one packet and then overwrite whatever is different between
     * any located objects.
     */
    mp.data.opcode = MTP_UPDATE_POSITION;
    mp.vers = MTP_VERSION;
    mp.role = MTP_ROLE_VMC;
    mup = &mp.data.mtp_payload_u.update_position;

    /*
     * XXX Find the index of the last valid objct so we know when to put
     * MTP_POSITION_STATUS_CYCLE_COMPLETE in the status field.
     */
    last_idx_set = 0;
    for (lpc = 0; lpc < mol->count; ++lpc) {
        if (mol->objects[lpc].valid) {
            last_idx_set = lpc;
        }
    }

    for (lpc = 0; lpc < mol->count; ++lpc) {
        if (mol->objects[lpc].valid) { /* Don't send meaningless data. */
	    if (debug > 1) {
		info("vmc-client: object[%d] - %f %f %f\n",
		     lpc,
		     mol->objects[lpc].px,
		     mol->objects[lpc].py,
		     mol->objects[lpc].pa);
	    }
	    
            mup->robot_id = -1;

	    /* Copy the camera-coordinates, then */
            mup->position.x = mol->objects[lpc].px;
            mup->position.y = mol->objects[lpc].py;
            mup->position.theta = mol->objects[lpc].pa;

	    radial_trans(&(mup->position));

	    /* ... transform them into global coordinates. */
            local2global_posit_trans(&(mup->position));

            if (lpc == last_idx_set) {
                /*
		 * Mark the end of updates for this frame so that vmcd can
		 * delete stale tracks.
		 */
                mup->status = MTP_POSITION_STATUS_CYCLE_COMPLETE;
            }
            else {
                mup->status = MTP_POSITION_STATUS_UNKNOWN;
            }

	    /*
	     * XXX We could probably just look at the timestamp to figure out
	     * when an update is for a different frame, instead of setting the
	     * MTP_POSITION_STATUS_CYCLE_COMPLETE flag.
	     */
            mup->position.timestamp = mm->time;

	    if (debug > 1) {
		mtp_print_packet(stdout, &mp);
	    }

	    /* Finally, encode the packet. */
            if (!xdr_mtp_packet(&xdr, &mp))
		assert(0);

	    xdrrec_endofrecord(&xdr, 1);
        }
    }

#if !defined(HAVE_MEZZANINE)
    mm->time += 1.0;
#endif

    retval = cursor - packet_buffer;

    if (retval == 0) {
	mtp_init_packet(&mp,
			MA_Opcode, MTP_CONTROL_ERROR,
			MA_Role, MTP_ROLE_VMC,
			MA_Code, MTP_POSITION_STATUS_CYCLE_COMPLETE,
			MA_Message, "",
			MA_TAG_DONE);
	
	if (!xdr_mtp_packet(&xdr, &mp))
	    assert(0);

	xdrrec_endofrecord(&xdr, 1);

	retval = cursor - packet_buffer;
    }

    if (debug > 1) {
	info("vmc-client: packet length %d\n", retval);
    }
    
    return retval;
}

int main(int argc, char *argv[])
{
    int c, port = VMCCLIENT_DEFAULT_PORT, serv_sock, on_off = 1;
    char *mezzfile, *logfile = NULL, *pidfile = NULL;
    mezz_mmap_t *mezzmap = NULL;
    int retval = EXIT_FAILURE;
    struct sockaddr_in sin;
    struct sigaction sa;

    xdrrec_create(&xdr, 0, 0, NULL, NULL, mem_write);
    
    while ((c = getopt(argc, argv, "hdp:l:i:f:x:y:")) != -1) {
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
        case 'p':
            if (sscanf(optarg, "%d", &port) != 1) {
                error("error: -p option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'f':
#if !defined(HAVE_MEZZANINE)
            if ((posfile = fopen(optarg, "r")) == NULL) {
                error("error: cannot open %s\n", optarg);
                usage();
                exit(1);
            }
#endif
            break;
        case 'x':
	    if (sscanf(optarg, "%lf", &x_offset) != 1) {
                error("error: -x option is not a number: %s\n", optarg);
                usage();
                exit(1);
	    }
            break;
        case 'y':
	    if (sscanf(optarg, "%lf", &y_offset) != 1) {
                error("error: -y option is not a number: %s\n", optarg);
                usage();
                exit(1);
	    }
            break;
	case 'z':
	    if (sscanf(optarg, "%lf", &z_offset) != 1) {
                error("error: -z option is not a number: %s\n", optarg);
                usage();
                exit(1);
	    }
	    break;
        default:
            break;
        }
    }
    
    argv += optind;
    argc -= optind;
    
    if (argc == 0) {
        error("error: missing mezzanine file argument\n");
        usage();
        exit(1);
    }
    
    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);

    signal(SIGPIPE, SIG_IGN);
    
    mezzfile = argv[0];
    
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
    
#if defined(HAVE_MEZZANINE)
    {
        if (mezz_init(0, mezzfile) == -1) {
            errorc("unable to initialize mezzanine\n");
            exit(2);
        }
        mezzmap = mezz_mmap();
    }
#else
    {
        mezzmap = calloc(1, sizeof(mezz_mmap_t));
        
        mezzmap->time = 0.0;
        mezzmap->objectlist.count = 2;
        
        mezzmap->objectlist.objects[0].valid = 1;
        mezzmap->objectlist.objects[0].px = 2.5;
        mezzmap->objectlist.objects[0].py = 5.5;
        mezzmap->objectlist.objects[0].pa = 0.48;
        
        mezzmap->objectlist.objects[1].valid = 1;
        mezzmap->objectlist.objects[1].px = 4.5;
        mezzmap->objectlist.objects[1].py = 6.5;
        mezzmap->objectlist.objects[1].pa = 0.54;
    }
#endif

    /*
     * Install our own SIGUSR1 handler _over_ the one installed by mezz_init,
     * so we can increment the local frame count and really be able to tell
     * when a frame was received.
     */
    sa.sa_handler = sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    
    if (pidfile) {
        FILE *fp;
        
        if ((fp = fopen(pidfile, "w")) != NULL) {
            fprintf(fp, "%d\n", getpid());
            (void) fclose(fp);
        }
    }
    
    memset(&sin, 0, sizeof(sin));
#ifndef linux
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    
    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error("cannot create socket\n");
    }
    else if (setsockopt(serv_sock,
			SOL_SOCKET,
			SO_REUSEADDR,
                        &on_off,
                        sizeof(on_off)) == -1) {
        errorc("setsockopt(SO_REUSEADDR)");
    }
    else if (bind(serv_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        errorc("bind");
    }
    else if (listen(serv_sock, 5) == -1) {
        errorc("listen");
    }
    else {
        fd_set readfds, clientfds;
        int last_mezz_frame = 0;
        
        FD_ZERO(&readfds);
        FD_ZERO(&clientfds); /* We'll track clients using this fd_set. */
        
        FD_SET(serv_sock, &readfds);

        while (looping) { /* The main loop. */
            fd_set rreadyfds;
            int rc;
            
            rreadyfds = readfds;

	    /*
	     * Block waiting for a SIGUSR1 signal or a file-descriptor to
	     * become read-ready.  Technically, we could miss a signal and a
	     * frame, but I'm too lazy to fix it at the moment.
	     */
            rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);
	    
            if (rc > 0) {
                int lpc;

		/* First, check for a client connection. */
                if (FD_ISSET(serv_sock, &rreadyfds)) {
                    struct sockaddr_in peer_sin;
                    socklen_t slen;
                    int fd;
                    
                    slen = sizeof(peer_sin);
                    if ((fd = accept(serv_sock,
                                     (struct sockaddr *)&peer_sin,
                                     &slen)) == -1) {
                        errorc("accept");
                    }
                    else {
			if (debug) {
			    info("vmc-client: connect from %s:%d (fd=%d)\n",
				 inet_ntoa(peer_sin.sin_addr),
				 ntohs(peer_sin.sin_port),
				 fd);
			}
			
			if (setsockopt(fd,
				       IPPROTO_TCP,
				       TCP_NODELAY,
				       &on_off,
				       sizeof(on_off)) == -1) {
			    errorc("setsockopt(TCP_NODELAY)");
			}
                        FD_SET(fd, &readfds);
                        FD_SET(fd, &clientfds);
                    }
                }
                
                /*
		 * We assume that if somebody connected to us tries to write
                 * us some data, they're screwing up; we're just a data source.
                 */
                for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
                    if ((lpc != serv_sock) && FD_ISSET(lpc, &rreadyfds)) {
			if (debug) {
			    char buffer[512];
			    int rc;

			    rc = read(lpc, buffer, sizeof(buffer));
			    if (rc == 0) {
				info("vmc-client: fd %d disconnected\n", lpc);
			    }
			    else {
				info("vmc-client: fd %d send %d bytes -- "
				     "disconnecting\n",
				     lpc,
				     rc);
			    }
			}
			
                        close(lpc);
                        FD_CLR(lpc, &readfds);
                        FD_CLR(lpc, &clientfds);
                    }
                }
            }
            else if (rc == -1) {
                switch (errno) {
                case EINTR:
                    /*
		     * Check the current frame count against the last one so we
		     * can tell if there really is a new frame ready.
                     */
                    if (mezz_frame_count != last_mezz_frame) {
			if (debug > 1) {
			    info("vmc-client: new frame\n");
			}
                        
                        if ((rc = encode_packets(mezzmap)) == -1) {
                            errorc("error: unable to encode packets");
                        }
                        else if (rc == 0) {
			    if (debug) {
				info("vmc-client: nothing to send %d\n",
				     mezz_frame_count);
			    }
			}
			else {
                            int lpc;
                            
                            for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
                                if (FD_ISSET(lpc, &clientfds)) {
                                    write(lpc, packet_buffer, rc);
                                }
                            }
                        }
			
                        last_mezz_frame = mezz_frame_count;
                    }
                    break;
                default:
                    errorc("unhandled select error\n");
                    break;
                }
            }
        }
    }
    
#if defined(HAVE_MEZZANINE)
    mezz_term(0);
#endif

    xdr_destroy(&xdr);
    
    return retval;
}
