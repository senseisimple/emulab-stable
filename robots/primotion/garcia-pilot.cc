/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file garcia-pilot.cc
 *
 * The main body of the daemon that runs on the garcia robot.
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ucontext.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <list>
#include <algorithm>

#include "acpGarcia.h"
#include "acpValue.h"

#include "garcia-pilot.hh"
#include "pilotClient.hh"
#include "dashboard.hh"
#include "wheelManager.hh"
#include "pilotButtonCallback.hh"

/**
 * The default port to listen for client connections.
 */
#define PILOT_PORT 2531

/**
 * Default path to the battery log.
 */
static const char *BATTERY_LOG_PATH = "/var/emulab/logs/battery.log";


/**
 * Default file to read in state data
 */
static const char *DEFAULT_SFILE = "sdata.txt";



/**
 * Determines whether or not to continue in the main loop.
 *
 * @see sigquit
 */
static volatile int looping = 1;

/**
 * Signal handler for SIGINT, SIGTERM, and SIGQUIT signals.  Sets looping to
 * zero and aborts() if it is called more than three times in case the program
 * is unable to return to the main loop.
 *
 * @param sig The actual signal number received.
 */
static void sigquit(int sig)
{
    static int quit_count = 3;
    
    if (quit_count == 0) {
	static char *msg = "error: wedged, aborting...\n";
        
	write(2, msg, strlen(msg));

	abort();
    }
    
    looping = 0;
    quit_count -= 1;
}

/**
 * Signal handler for SIGUSR1, just toggles the debugging level.
 *
 * @param sig The actual signal number received.
 */
static void sigdebug(int sig)
{
    if (debug == 3)
	debug = 1;
    else
	debug = 3;
}

/**
 * Signal handler for SIGSEGV and SIGBUS, just prints out the signal number and
 * aborts.
 */
static void sigpanic(int signum)
{
    char msg[128];
    
    snprintf(msg, sizeof(msg), "panic: %d; reexec'ing\n", signum);
    write(1, msg, strlen(msg));

#if 0
    execve(reexec_argv[0], reexec_argv, environ);

    snprintf(msg, sizeof(msg), "reexec failed %s\n", strerror(errno));
    write(1, msg, strlen(msg));
#endif

    abort();
}

/**
 * Prints the usage message for this daemon to stderr.
 */
static void usage(void)
{
    fprintf(stderr,
	    "usage: garcia-pilot -hd [-l logfile] [-i pidfile] [-p port]\n"
	    "  [-b battery-log]\n"
	    "Options:\n"
	    "  -h\t\tPrint this message\n"
	    "  -d\t\tTurn on debugging messages and do not daemonize\n"
	    "  -o\t\tOpen loop path demo\n"
	    "  -l logfile\tSend log output to the given file\n"
	    "  -i pidfile\tWrite the process ID to the given file\n"
	    "  -p port\tListen on the given port for MTP messages\n"
	    "  -b battery-log\tAppend battery log data to the given file\n"
	    "    \t\t(Default: %s)\n",
	    BATTERY_LOG_PATH);
}

class pilotFaultCallback : public faultCallback
{
    
public:
    
    pilotFaultCallback(wheelManager &wm) : pfc_wheel_manager(wm) { };

    void faultDetected(unsigned long faults)
    {
	this->pfc_wheel_manager.stop();
    };

private:

    wheelManager &pfc_wheel_manager;
    
};



int main(int argc, char *argv[])
{
    int c, port = PILOT_PORT, serv_sock, on_off = 1, ol_demo = 0;
    const char *logfile = NULL, *plogfile = NULL, *pidfile = NULL;
    const char *batteryfile = BATTERY_LOG_PATH;
    const char *sfile = DEFAULT_SFILE;
    int retval = EXIT_SUCCESS;
    unsigned long now, t_offset, t_elapsed, ti_start;
    unsigned long ti_list[1000]; /* list of iteration elapsed times */
    
    float ti_mean, ti_var;
    
    struct timeval tv_last = {0,0};
    struct timeval tv_cur = {0,0};
    
    double tf_last, tf_cur, tf_diff;
    
    FILE *batterylog;
    FILE *sdata_in;
    FILE *plogfilep;
    aIOLib ioRef;
    aErr err;

    while ((c = getopt(argc, argv, "hdop:l:k:i:b:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'o':
	    ol_demo = 1;
	    break;
	case 'l':
	    logfile = optarg;
	    break;
	case 'k':
	    plogfile = optarg;
	    break;
	case 'i':
	    pidfile = optarg;
	    break;
	case 'p':
	    if (sscanf(optarg, "%d", &port) != 1) {
		fprintf(stderr,
			"error: -p option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'b':
	    batteryfile = optarg;
	    break;
	}
    }

#if 0
    if (getuid() != 0) {
	fprintf(stderr, "error: must run as root!\n");
	exit(1);
    }
#endif

    if (!debug) {
	/* Become a daemon */
	daemon(1, 0);
    }

#if 0
    /* Bump the priority so it has a good chance of receiving packets. */
    {
	struct sched_param sp;
	
	sp.sched_priority = sched_get_priority_min(SCHED_RR);
	if (sched_setscheduler(0, SCHED_RR, &sp) < 0) {
	    fprintf(stderr, "warning: cannot set real-time priority\n");
	}
    }
#endif
    
    if (logfile) {
	int logfd;
        
	if ((logfd = open(logfile, O_CREAT|O_WRONLY|O_APPEND, 0644)) != -1) {
	    dup2(logfd, 1);
	    dup2(logfd, 2);
	    if (logfd > 2)
		close(logfd);
	}
    }
    
    if (pidfile) {
	FILE *fp;
        
	if ((fp = fopen(pidfile, "w")) != NULL) {
	    (void) fclose(fp);
	}
    }

    if ((batterylog = fopen(batteryfile, "a")) == NULL) {
	fprintf(stderr,
		"error: cannot open battery log file - %s\n",
		batteryfile);
	exit(1);
    }

    if (plogfile) {
	if ((plogfilep = fopen(plogfile, "w")) == NULL) {
	    fprintf(stderr, "ERROR: cannot open packet log file, %s\n", plogfile);
	}
    }
    
    
    
    fprintf(stderr, "info: %s\n", build_info);

    acpGarcia garcia;
    
    signal(SIGHUP, sigdebug);

    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);

    signal(SIGSEGV, sigpanic);
    signal(SIGBUS, sigpanic);
    
    signal(SIGPIPE, SIG_IGN);
    
    aIO_GetLibRef(&ioRef, &err);

    if (!wait_for_brainstem_link(ioRef, garcia)) {
	fprintf(stderr,
		"error: could not connect to robot %d\n",
		garcia.getNamedValue("status")->getIntVal());
	exit(1);
    }
    else {
	struct sockaddr_in saddr;
	acpValue av;

	if (debug)
	    fprintf(stderr, "debug: connected to brainstem\n");

	/* Make sure the units used by the robot are what we expect. */
	av.set("radians");
	garcia.setNamedValue("angle-units-string", &av);

	av.set("meters");
	garcia.setNamedValue("distance-units-string", &av);

	/* turn off fall sensors */
	/* WHY?? -- Dan */
	av.set(0);
	garcia.setNamedValue("down-ranger-enable", &av);

	/* set the stall threshhold high */
	av.set(12.0f);
	garcia.setNamedValue("stall-threshhold", &av);
      
	/*
	 * XXX Clear these values since they can be left in a funny state
	 * sometimes...
	 */
	av.set(0);
	garcia.setNamedValue("distance-left", &av);
	garcia.setNamedValue("distance-right", &av);
	av.set(aGARCIA_ERRFLAG_ABORT);
	garcia.setNamedValue("status", &av);
	
	memset(&saddr, 0, sizeof(saddr));

	
	if (1 == ol_demo) {
	    /* open loop demonstration mode */
          
	    float e_in = 0.0f, alpha_in = 0.0f, theta_in = 0.0f; /* states */
	    float C_u, C_omega; /* controller outputs */
          
	    /* controller parameters: */
	    float K_gamma = 1.0f, K_h = 1.0f, K_k = 1.0f;
          
	    float K_radius = 0.0889f;
	    float vleft, vright;
	    acpValue av_L;
	    acpValue av_R;
          
	    acpObject *nullb;  
          
	    int lcount = 0;
          
          
          
	    if ((sdata_in = fopen(sfile, "r")) == NULL) {
		fprintf(stderr,
			"FATAL: Error opening state input file:  %s\n",
			sfile);
		exit(1);
	    }
          
	    aIO_GetMSTicks(ioRef, &t_offset, NULL);
          
          
        
	    nullb = garcia.createNamedBehavior("null", NULL);
	    av.set(0.2f);
	    nullb->setNamedValue("acceleration", &av);
          
	    garcia.queueBehavior(nullb);
          

	    /* read states (e,alpha,theta) in from file: */
	    /* These states will come from the RMCD/EMCD in the future */
	    while (3 == fscanf(sdata_in, "%f %f %f\n", &e_in, &alpha_in, &theta_in)) {
          

		aIO_GetMSTicks(ioRef, &ti_start, NULL);
            
		/* controller: */
		C_u = 0.8 * tanh(K_gamma * cos(alpha_in) * e_in);
            
		if (0 == alpha_in) {
		    C_omega = 0.0f;
		}
		else {
		    C_omega = K_k * alpha_in + K_gamma * ((cos(alpha_in)*sin(alpha_in)) / alpha_in) * (alpha_in + K_h * theta_in);
              
		    if (C_omega > 26.25f) {
			C_omega = 26.25f;
		    }
		    if (C_omega < -26.25f) {
			C_omega = -26.25f;
		    }
		}
		/* end controller */
            
		/* wheel velocity translator: */
		vleft = C_u - K_radius * C_omega;
		vright = C_u + K_radius * C_omega;
		/* end wheel velocity translator */
            
            
		av_L.set(vleft);
		av_R.set(vright);
            
		garcia.setNamedValue("damped-speed-left", &av_L);
		garcia.setNamedValue("damped-speed-right", &av_R);
            
		//             vleft = garcia.getNamedValue("damped-speed-left")->getFloatVal();
		//             vright = garcia.getNamedValue("damped-speed-right")->getFloatVal();
            
		/*            if (debug) {
			      fprintf(stderr, "L/R velocities: %f %f\n", vleft, vright);
			      }*/
            
            
		/* wait 0.033 (1/30) seconds */
		garcia.handleCallbacks(33);
		aIO_GetMSTicks(ioRef, &now, NULL);
		ti_list[lcount] = now - ti_start;
            
		lcount++;          
	    }
          
          
	    /* abort the null primitive: */
	    av.set(aGARCIA_ERRFLAG_ABORT);
	    garcia.setNamedValue("status", &av);
          
          
	    if (sdata_in != NULL) {
		fclose(sdata_in);
		sdata_in = NULL;
	    }
        
	    aIO_GetMSTicks(ioRef, &now, NULL);
	    t_elapsed = now - t_offset;
          
          
          
	    ti_mean = 0.0f;
	    ti_var = 0.0f;
	    for (int incr_i = 0; incr_i < lcount; incr_i++) {
		ti_mean += (float)(ti_list[incr_i]);
		ti_var += (float)(ti_list[incr_i]) * (float)(ti_list[incr_i]);
	    }
	    ti_var = (ti_var - ti_mean*ti_mean / ((float)(lcount))) / ((float)(lcount));
	    ti_mean = ti_mean / ((float)(lcount));
          
          
	    if (debug) {
		fprintf(stderr, "Done with file: read %d lines\n", lcount);
		fprintf(stderr, "Elapsed time: %d\n", t_elapsed);
		fprintf(stderr, "Iteration times:\n mean: %f, variance: %f\n", ti_mean, ti_var);
	    }
          
	    exit(0);
	}
	
	memset(&saddr, 0, sizeof(saddr));
#if !defined(linux)
	saddr.sin_len = sizeof(saddr);
#endif
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = INADDR_ANY;
	
	if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket");
	}
	else if (setsockopt(serv_sock,
			    SOL_SOCKET,
			    SO_REUSEADDR,
			    &on_off,
			    sizeof(on_off)) == -1) {
	    perror("setsockopt");
	}
	else if (bind(serv_sock,
		      (struct sockaddr *)&saddr,
		      sizeof(saddr)) == -1) {
	    perror("bind");
	}
	else if (listen(serv_sock, 5) == -1) {
	    perror("listen");
	}
	else {
	    pilotClient::list clients;
	    fd_set readfds, writefds;
	    
	    dashboard db(garcia, batterylog);
	    wheelManager wm(garcia);
	    pilotButtonCallback pbc(db, wm);
	    pilotFaultCallback pfc(wm);
	    
	    FD_ZERO(&readfds);
	    FD_ZERO(&writefds);
	    FD_SET(serv_sock, &readfds);

	    db.setFaultCallback(&pfc);
	    wm.setDashboard(&db);
	    do {
		fd_set rreadyfds = readfds, wreadyfds = writefds;
		struct timeval tv_zero = { 0, 10000 };
		int rc;

		/* Poll the file descriptors, don't block */
		rc = select(FD_SETSIZE,
			    &rreadyfds,
			    &wreadyfds,
			    NULL,
			    &tv_zero);
		if (rc > 0) {
		    bool do_telem = db.wasTelemetryUpdated();
		    pilotClient::iterator i, j;
		    struct mtp_packet tmp;
		    
		    if (FD_ISSET(serv_sock, &rreadyfds)) {
			struct sockaddr_in peer_sin;
			socklen_t slen;
			int cfd;
			
			slen = sizeof(peer_sin);
			if ((cfd = accept(serv_sock,
					  (struct sockaddr *)&peer_sin,
					  &slen)) == -1) {
			    perror("accept");
			}
			else {
			    pilotClient *pc = new pilotClient(cfd, wm, db);
			    
			    if (debug) {
				fprintf(stderr,
					"debug: connect from %s:%d\n",
					inet_ntoa(peer_sin.sin_addr),
					ntohs(peer_sin.sin_port));
			    }
			    
			    pc->setFD(&readfds);
			    pc->setFD(&writefds);
			    clients.push_back(pc);
			}
		    }
		    
		    if (do_telem) {
			mtp_init_packet(&tmp,
					MA_Opcode, MTP_TELEMETRY,
					MA_Role, MTP_ROLE_RMC,
					MA_GarciaTelemetry, db.getTelemetry(),
					MA_TAG_DONE);
		    }

		    for (i = clients.begin(); i != clients.end(); ) {
			bool bad_client = false;
			pilotClient *pc = *i;
			
			j = i++;
			if (pc->isFDSet(&rreadyfds)) {
			    do {
				struct mtp_packet mp;
				
				if ((mtp_receive_packet(pc->getHandle(),
							&mp) !=
				     MTP_PP_SUCCESS) ||
				    !pc->handlePacket(&mp, clients)) {
				    bad_client = true;
				} else {
      
				    if (MTP_COMMAND_WHEELS == mp.data.opcode) {
					/* timestamp */
                                        
					tv_last = tv_cur;
					gettimeofday(&tv_cur, NULL);
            
					tf_last = (double)(tv_last.tv_sec) + (double)(tv_last.tv_usec) / 1000000.0;
					tf_cur = (double)(tv_cur.tv_sec) + (double)(tv_cur.tv_usec) / 1000000.0;
            
					tf_diff = tf_cur - tf_last;
            
					if (plogfilep != NULL) {
					    fprintf(plogfilep, "%f\n", tf_diff);
					}
				    }
				}
				
				mtp_free_packet(&mp);
			    } while (!bad_client &&
				     pc->getHandle()->mh_remaining);
			}

			if (!bad_client &&
			    (pc->getRole() == MTP_ROLE_EMULAB) &&
			    pc->isFDSet(&wreadyfds)) {
			    if (do_telem &&
				mtp_send_packet(pc->getHandle(), &tmp) !=
				MTP_PP_SUCCESS) {
				fprintf(stderr,
					"error: cannot send telemetry to "
					"client\n");

				bad_client = true;
			    }
			}
			
			if (bad_client) {
			    clients.erase(j);
			    
			    pc->clearFD(&readfds);
			    pc->clearFD(&writefds);
			    
			    delete pc;
			    pc = NULL;
			}
		    }
		}
		
		garcia.handleCallbacks(23);
		aIO_GetMSTicks(ioRef, &now, NULL);
	    } while (looping && db.update(now));

	    garcia.flushQueuedBehaviors();
	    
	    garcia.handleCallbacks(1000);
	}
    }

    aIO_ReleaseLibRef(ioRef, &err);

    if (batterylog != NULL) {
	fclose(batterylog);
	batterylog = NULL;
    }
    
    return retval;
}
