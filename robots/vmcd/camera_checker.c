/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* camera_checker - Automated check system using fixed permanent fiducials.
 * Re-uses the framework of file_dumper to connect to Mezzanine.
 * 
 * Monitors the fiducial positions continuously, reporting after a pre-set
 * interval when things go bad.  Its job is to determine whether a camera has
 * become mis-aligned, and allow corrective action in the short term.
 * 
 * Since Mezzanine reports all of the fiducials in the field of view of the
 * camera, it just ignores everything outside a small distance from the known
 * fixed fiducial location.  Ignore short-term loss of signal (somebody
 * walking over the fiducial) or displacements (robot obscuring the fixed
 * fiducial.)
 * 
 * Log the fiducial position once in a while, to verify that the checker is
 * working, and whether a trend is (or has been) developing but still
 * sub-alarm.
 *
 * This is the algorithm:
 *  - Once a minute, gather 5 frames of data and average them for each
 *    visible fiducial.
 *
 *  - Look for a fiducial object within a small pixel tolerance of the center
 *    point.
 *
 *  - Log failures, reporting the closest out-of-tolerance fiducial, if any.
 *
 *  - E-mail to testbed-robocops after failing solidly for some time.
 *    Only send during weekday operation hours, and back off exponentially.
 *
 *  - Log a successful sample once an hour for history. 
 *
 * All of the constants are configurable by command-line args.  */

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

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "log.h"
//#include "mtp.h"
#include <math.h>

#include "mezz.h"


static volatile unsigned int mezz_frame_count = 0;
/**
 * Signal handler for SIGUSR1 that updates the mezzanine frame count so the
 * main loop knows there is a new frame available.
 *
 * @param signal The actual signal received.  */
static void sigusr1(int signal)
{
    assert(signal == SIGUSR1);
    
    mezz_frame_count += 1;
}

static char *alarm_addr = "testbed-robocops@flux.utah.edu";
static char *dbg_addr = "$USER@flux.cs.utah.edu";

/* Command-line args. */
static int debug = 0;
static char *cam_num = NULL;		/* Camera number for messages. */

static char *option_file = NULL;	/* Mezzanine option file for camera. */
int ctr_x = 0, ctr_y = 0;	/* Camera center pt in pixels from optfile. */
static char *log_file = NULL;		/* Logfile to append to. */
static FILE *option_FILE, *log_FILE;
static char *mezz_file = NULL;		/* Mezzanine IPC file  */

static int num_frames = 5;		/* Number of frames to average. */
static int frame_interval = 1;		/* Frames to skip in between. */
static int error_threshold = 5;		/* Pixel error before alarm. */
static int check_secs = 60;		/* Delay between checks. */
static int log_skip = 60;		/* Good checks to skip between logs. */
static int alarm_threshold = 10;	/* How many bad checks before alarm. */
static int alarm_start_hour = 8;	/* Beginning of daily operation time. */
static int alarm_end_hour = 18;		/* End of daily operation time. */
static float alarm_backoff_exponent=3.0; /* Multiplier for alarm threshold. */

void usage() {
    printf("camera_checker [-d] -C cam_num -f mezz_opt_file [-F logfile]\n");
  printf("	[-i skip_frames] [-n num_frames]\n");
  printf("	[-e error_threshold] [-c check_secs] [-l log_skip]\n");
  printf("	[-a alarm_threshold] [-m alarm_mail_addr]\n");
  printf("	[-S alarm_start_hour] [-E alarm_end_hour]\n");
  printf("	[-B alarm_backoff_exponent] mezz_ipc_file\n");
}

static char tsbuff[256];
char *timestamp() {
    time_t now;
    time(&now);
    char * t = ctime(&now);	/* Fixed format: Thu Nov 24 18:22:48 1986\n\0 */
    t[24] = '\0';			/* Kill the newline. */
    snprintf(tsbuff,256,(cam_num?"[%s camera %s]":"[%s]"),t,cam_num);
    return tsbuff;
}

int main(int argc, char *argv[])
{
    int c;
    mezz_mmap_t *mezzmap = NULL;
    struct sigaction sa;

    while ((c = getopt(argc, argv, "dC:f:F:n:i:e:c:l:a:m:S:E:B:")) != -1) {
        switch (c) {
        case 'd':
            debug = 1;
	    check_secs = 1;
	    log_skip = 1;
	case 'f':
	    // mezzanine.opt.cameraN file, to get dewarp.cameraCenter parameters.
	    option_file = optarg;
	    break;
        case 'C':
            cam_num = optarg;
            break;
	case 'F':
	    // Log file name for this camera.
	    log_file = optarg;
	    break;
        case 'i':
            frame_interval = atof(optarg);
	    if (frame_interval < 1) {
		error("error: option -i is not a number > 0: %s\n",optarg);
		usage();
		exit(1);
	    }
            break;
        case 'n':
            if (sscanf(optarg, "%d", &num_frames) != 1) {
                error("error: -n option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'e':
            if (sscanf(optarg, "%d", &error_threshold) != 1) {
                error("error: -e option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'c':
            if (sscanf(optarg, "%d", &check_secs) != 1) {
                error("error: -c option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'l':
            if (sscanf(optarg, "%d", &log_skip) != 1) {
                error("error: -l option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'a':
            if (sscanf(optarg, "%d", &alarm_threshold) != 1) {
                error("error: -a option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'm':
            alarm_addr = optarg;
            break;
        case 'S':
            if (sscanf(optarg, "%d", &alarm_start_hour) != 1) {
                error("error: -S option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'E':
            if (sscanf(optarg, "%d", &alarm_end_hour) != 1) {
                error("error: -E option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'B':
            if (sscanf(optarg, "%f", &alarm_backoff_exponent) != 1) {
                error("error: -laoption is not a real number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case '?':
	    usage();
	    exit(1);
            break;
        default:
            break;
        }
    }
    
    argv += optind;
    argc -= optind;
    
    if (option_file == NULL) {
        error("*** Missing mezzanine option file arg.\n");
        usage();
        exit(1);
    }


    if (option_file == NULL) {
        error("*** Missing mezzanine option file arg.\n");
        usage();
        exit(1);
    }

    if (argc < 1) {
        error("*** Missing mezzanine ipc file.\n");
        usage();
        exit(1);
    }
    else {
	mezz_file = argv[0];
    }

    signal(SIGPIPE, SIG_IGN);
    
    if (mezz_init(0, mezz_file) == -1) {
	error("unable to initialize mezzanine\n");
	//errorc("unable to initialize mezzanine\n");
	exit(2);
    }
    mezzmap = mezz_mmap();
    assert(mezzmap != NULL);
    if (debug) printf("Mezzmap %x\n",(unsigned int)mezzmap);
    
    unsigned int frames_needed;
    int i = 0, j = 0;

    /* Read the camera center coordinates from the mezzanine option file. */
    if (option_file != NULL) {
	option_FILE = fopen(option_file,"r");
	if (option_FILE != NULL) {
	    do {
		static char line[BUFSIZ];
		if (fgets(line,BUFSIZ,option_FILE)) {
		    i = sscanf(line,"dewarp.cameraCenter = ( %d, %d )",
			       &ctr_x,&ctr_y);
		    // if (debug) printf("%d %s\n", i, line);
		}
	    } while (!feof(option_FILE) && i != 2);
	    fclose(option_FILE);
	    if ( i != 2 ) {
		    error("Couldn't find dewarp.cameraCenter in %s.\n",
			  option_file);
		    exit(1);
	    }
	    printf("ctr_x %d, ctr_y %d\n",ctr_x,ctr_y);
	}
    }

    if (log_file != NULL) {
	log_FILE = fopen(log_file,"a");
	if (log_FILE == NULL) {
	    error("error: could not open output file for appending\n");
	    exit(1);
	}
	setlinebuf(log_FILE);
    }
    else {
	log_FILE = stdout;
    }

    fprintf(log_FILE,
	    "%s Starting, center (%d, %d), n %d, i %d, e %d, c %d, l %d\n", 
	    timestamp(),ctr_x,ctr_y,num_frames,frame_interval,
	    error_threshold, check_secs, log_skip);

    /*
     * Install our own SIGUSR1 handler _over_ the one installed by mezz_init,
     * so we can increment the local frame count and really be able to tell
     * when a frame was received.
     */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    int obj_warned = 0;
    int checks_skipped = log_skip;	/* Print the first one. */
    int bad_checks = 0;
    int alarm_delay = alarm_threshold;
    int last_frame;
    while (1) { /* The main loop. */
	/* get frame data */
	/* let signals in... */
	/* bit of a race here, but should be no prob */
	last_frame = mezz_frame_count = 0;
	frames_needed = num_frames;

	sa.sa_handler = sigusr1;
	sigaction(SIGUSR1, &sa, NULL);

	/* Average measurements across many frames for stability. */
#	define MAX_OBJS 100
	int n_objs = 0;
	int obj_num[MAX_OBJS], n_samples[MAX_OBJS];
	float total_x[MAX_OBJS], total_y[MAX_OBJS];
	while (frames_needed > 0) {

	    /* one second should always be plently of time to get
	     * a SIGUSR1
	     */
	    sleep(1);

	    /* disable mezz interrupts */
	    //sa.sa_handler = SIG_IGN;
	    //sigaction(SIGUSR1,&sa,NULL);

	    if (mezz_frame_count >= (last_frame + frame_interval)) {
		/* new frame */
		if (mezz_frame_count != 
		    (last_frame + frame_interval)
		    ) {
		    // must have missed one!
		    fprintf(log_FILE,"WARNING: missed %d frames!\n",
			    (mezz_frame_count - (last_frame +
						 frame_interval))
			    );
		}
		last_frame = mezz_frame_count;
		--frames_needed;

		/* Handle the objects in one frame of data from Mezzanine. */
		for (i = 0; i < mezzmap->objectlist.count; ++i) {
		    mezz_object_t *obj = &(mezzmap->objectlist.objects[i]);
		    if (obj->valid) {
			if (0 && debug) printf(
			    "[%d] a(%f,%f) b(%f,%f)\n", i, obj->ablob.ox, 
			    obj->ablob.oy, obj->bblob.ox, obj->bblob.oy);

			/* Accumulate coords from one object across frames. */
			for (j = 0; j < n_objs; j++)
			    if (obj_num[j] == i) break;
			if (j < MAX_OBJS) {
			    float fid_x = (obj->ablob.ox + obj->bblob.ox)/2.0;
			    float fid_y = (obj->ablob.oy + obj->bblob.oy)/2.0;
			    if (j == n_objs) {
				n_objs = j+1;
				obj_num[j] = i;
				n_samples[j] = 1;
				total_x[j] = fid_x;
				total_y[j] = fid_y;
			    }
			    else {
				n_samples[j]++;
				total_x[j] += fid_x;
				total_y[j] += fid_y;
			    }
			}
			else {
			    if (! obj_warned)
				fprintf(log_FILE,
					"WARNING: not enough obj space.\n");
			    obj_warned = 1;
			}
		    }
		}
	    }

	    /* reenable interrupts */
	    //sa.sa_handler = sigusr1;
	    //sigaction(SIGUSR1,&sa,NULL);

	}

	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1,&sa,NULL);

	/* Look for a fiducial within tolerance of the center point.  This is
	 * meant to be a small number of pixels, so there will be one or none.
	 */
	int got_fid = 0;
	float fid_x = 0, fid_y = 0, dist = 0;
#	define FAR 99999.0
	float close_x = 0, close_y = 0, closest = FAR;
	if (debug) printf("n_objs %d\n",n_objs);
	for (j = 0; j < n_objs; j++) {
	    fid_x = total_x[j] / n_samples[j];
	    fid_y = total_y[j] / n_samples[j];
	    dist = sqrt(pow(fid_x - ctr_x,2) + pow(fid_y - ctr_y,2));
	    if (dist < closest) {
		closest = dist;
		close_x = fid_x;
		close_y = fid_y;
	    }
	    if (dist < error_threshold) {
		got_fid = 1;
		if (debug) 
		    printf("Good fiducial at (%f, %f), distance %f, %d samples.\n",
			   fid_x,fid_y,dist,n_samples[j]);
		break;
	    }
	}
	if (debug) printf("got_fid %d, fid %f %f, closest %f, close %f %f\n",
			  got_fid,fid_x,fid_y,closest,close_x,close_y);

	if (got_fid) {
	    /* Log every Nth successful check. */
	    if (checks_skipped++ >= log_skip) {
		fprintf(log_FILE,
			"%s fiducial (%f, %f), distance %f pixels.\n",
			timestamp(),fid_x,fid_y,dist);
		checks_skipped = 0;
	    }
	    bad_checks = 0;
	}
	else {
	    /* Log all errors. */
	    char msg_buff[256];
	    if (closest != FAR)
		snprintf(msg_buff,256,
			 "%s *** MISSED: closest (%f, %f), distance %f pixels.",
			 timestamp(),close_x,close_y,closest);
	    else
		snprintf(msg_buff,256,
			 "%s *** NO FIDUCIAL VISIBLE.",
			 timestamp());
	    fprintf(log_FILE,"%s\n",msg_buff);

	    /* No alarms except during operating hours on week days. */
	    char *now = timestamp();  /* E.g. "Thu Nov 24 18:22:48 1986\0" */
	    int hour;
	    if (sscanf(now,"%*s %*s %*d %d:", &hour) != 1)
		fprintf(log_FILE,"***** Failed to parse hour from %s\n", now);
	    int early = hour<alarm_start_hour, late = hour>=alarm_end_hour;
	    int op_hr = now[0]!='S' && !early && !late;	/* No weekends. */
	    if (debug) printf("now %s, hour %d, early %d, late %d, op_hr %d\n",
			      now,hour,early,late,op_hr);
	    if (early || late) alarm_delay = alarm_threshold; /* Reset. */

	    /* Don't raise the alarm unless things have been bad for a while. */
	    if (op_hr && bad_checks++ >= alarm_delay) {
		char mail_buff[256];
		char *to = (debug?dbg_addr:alarm_addr);
		snprintf(mail_buff,256,
			 "mail %s -s '[Robocops] Camera check %s' %s </dev/null",
			 (0 && debug?"-v":""),msg_buff, to);

		/* Don't whine constantly, and back off exponentially. */
		bad_checks = 0;
		alarm_delay = (int)powf(alarm_delay, alarm_backoff_exponent);
		fprintf(log_FILE,
			"%s ***** Alarm mail sent, status %d, next delay %d.\n",
			timestamp(),system(mail_buff), alarm_delay);
	    }
	}

	sleep(check_secs);
    }
    
    mezz_term(0);

    fprintf(log_FILE,
	    "%s Stopping, center (%d, %d), n %d, i %d, e %d, c %d, l %d\n", 
	    timestamp(),ctr_x,ctr_y,num_frames,frame_interval,
	    error_threshold, check_secs, log_skip);
}
