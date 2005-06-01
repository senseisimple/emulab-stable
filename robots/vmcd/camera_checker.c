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
 *    Don't log "No Fiducial Visible" at night when the lights may be off.
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
/* Can't use $USER while debugging, sudo sets it to root. */
static char *dbg_addr = "fish@cs.utah.edu";

int got_ctr = 0;	    /* Required camera center pt from option file. */
int got_chk = 0;	    /* Optional camera check pt from option file. */
#define NPTS 2
char *pt_names[NPTS] = {"Center point","Check point"} ;
#define CTR_PT 0
#define CHK_PT 1
int pts[NPTS][2];		    /* Pixel coords of option file points. */
#define X 0
#define Y 1

/* Command-line args. */
static int debug = 0;
static int show_all_fids = 0;	    /* Print all visible fiducial locs. */

static char *cam_num = NULL;	    /* Camera number for messages. */
static char *option_file = NULL;    /* Mezzanine option file for camera. */
static char *log_file = NULL;	    /* Logfile to append to. */
static FILE *option_FILE, *log_FILE;
static char *mezz_file = NULL;	    /* Mezzanine IPC file  */

static int num_frames = 5;	    /* Number of frames to average. */
static int frame_interval = 1;	    /* Frames to skip in between. */
static int error_threshold = 5;	    /* Pixel error before alarm. */
static int check_secs = 60;	    /* Delay between checks. */
static int log_skip = 60;	    /* Good checks to skip between logs. */

static int alarm_threshold = 10;    /* How many bad checks before alarm. */
static int alarm_start_hour = 8;    /* Beginning of daily operation time. */
static int alarm_end_hour = 18;	    /* End of daily operation time. */
static float alarm_backoff_exponent=3.0; /* Multiplier for alarm threshold. */

void usage() {
    printf("camera_checker [-da] -C cam_num -f mezz_opt_file [-F logfile]\n");
  printf("	[-i skip_frames] [-n num_frames]\n");
  printf("	[-e error_threshold] [-c check_secs] [-l log_skip]\n");
  printf("	[-A alarm_threshold] [-M alarm_mail_addr]\n");
  printf("	[-S alarm_start_hour] [-E alarm_end_hour]\n");
  printf("	[-B alarm_backoff_exponent] mezz_ipc_file\n");
}

#define LITTLE_BUFSIZ 256
static char tsbuff[LITTLE_BUFSIZ];
char *timestamp() {
    time_t now;
    time(&now);
    char * t = ctime(&now);	/* Fixed format: Thu Nov 24 18:22:48 1986\n\0 */
    t[24] = '\0';			/* Kill the newline. */
    snprintf(tsbuff,LITTLE_BUFSIZ,(cam_num?"[%s camera %s]":"[%s]"),t,cam_num);
    return tsbuff;
}

/* Last-known-good time for e-mail. */
static char *good_ts = "[Never]";
static char good_ts_buff[LITTLE_BUFSIZ];

int main(int argc, char *argv[])
{
    int c;
    mezz_mmap_t *mezzmap = NULL;
    struct sigaction sa;

    while ((c = getopt(argc, argv, "daC:f:F:i:n:e:c:l:A:M:S:E:B:")) != -1) {
        switch (c) {
        case 'd':
            debug = 1;
	    check_secs = 1;
	    log_skip = 1;
	    break;
        case 'a':
	    show_all_fids = 1;
	    break;
        case 'C':
            cam_num = optarg;
            break;
	case 'f':
	    // mezzanine.opt.cameraN file, to get dewarp.cameraCenter parameters.
	    option_file = optarg;
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
        case 'A':
            if (sscanf(optarg, "%d", &alarm_threshold) != 1) {
                error("error: -a option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        case 'M':
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
    int i = 0, j = 0, k = 0;

    /* Read the camera center coordinates from the mezzanine option file. */
    if (option_file != NULL) {
	option_FILE = fopen(option_file,"r");
	if (option_FILE != NULL) {
	    do {
		/* Looking for dewarp.cameraCenter and dewarp.cameraCheck */
		static char line[BUFSIZ], name[BUFSIZ];
		int coords[2], pt = -1;
		if (fgets(line,BUFSIZ,option_FILE)) {
		    i = sscanf(line,"dewarp.%s = ( %d, %d )",
			       name,&coords[X],&coords[Y]);
		    // if (debug) printf("%d %s\n", i, line);
		}
		if ( i != 3 )
		    continue;
		if (strcmp(name,"cameraCenter")==0) {
		    /* Required camera center pt from optfile, pixel coords. */
		    got_ctr = 1;
		    pt = CTR_PT;
		}
		else if (strcmp(name,"cameraCheck")==0) {
		    /* Optional camera check pt from optfile, pixel coords. */
		    got_chk = 1;
		    pt = CHK_PT;
		}
		if (pt != -1) {
		    for (k=0; k<2; k++) 
			pts[pt][k] = coords[k];
		    printf("%s (%d, %d)\n",
			   pt_names[pt],pts[pt][X],pts[pt][Y]);
		}
	    } while (!feof(option_FILE));
	    fclose(option_FILE);
	}
    }
    if (!got_ctr) {
	error("Couldn't find dewarp.cameraCenter in %s.\n", option_file);
	exit(1);
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
	    "%s Starting, pts (%d, %d) (%d, %d), n %d, i %d, e %d, c %d, l %d\n", 
	    timestamp(),
	    pts[CTR_PT][X],pts[CTR_PT][Y],
	    pts[CHK_PT][X],pts[CHK_PT][Y],
	    num_frames,frame_interval, error_threshold, check_secs, log_skip);

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
    int no_fid_msg_count = 0;
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

	/* No alarms except during operating hours on week days. */
	char *now = timestamp();  /* E.g. "Thu Nov 24 18:22:48 1986\0" */
	int hour;
	if (sscanf(now,"%*s %*s %*d %d:", &hour) != 1)
	    fprintf(log_FILE,"***** Failed to parse hour from %s\n", now);
	int early = hour<alarm_start_hour, late = hour>=alarm_end_hour;
	int op_hrs = now[1]!='S' && !early && !late;	/* No weekends. */
	if (debug) printf("now %s, hour %d, early %d, late %d, op_hrs %d\n",
			  now,hour,early,late,op_hrs);
	if (early || late) alarm_delay = alarm_threshold; /* Reset. */

	/* Look for fiducials within tolerance of the center and check points.
	 * The tolerance is meant to be a small number of pixels, so there
	 * will be one or none matching each.
	 */
	int got_fid, fid_cnt = 0, do_log = 0, n_msgs = 0;
	static char msgs[NPTS+1][LITTLE_BUFSIZ];
	for (i=0; i<=got_chk; i++) {
	    float fid_x = 0, fid_y = 0, dist = 0;
#	    define FAR 99999.0
	    float close_x = 0, close_y = 0, closest = FAR;
	    got_fid = 0;
	    if (debug) printf("n_objs %d\n",n_objs);
	    for (j = 0; j < n_objs; j++) {
		fid_x = total_x[j] / n_samples[j];
		fid_y = total_y[j] / n_samples[j];
		if (show_all_fids)
		    printf("Obj %d coords (%f, %f)\n",j,fid_x,fid_y);
		dist = sqrt(pow(fid_x - pts[i][X],2) + 
			    pow(fid_y - pts[i][Y],2));
		if (dist < closest) {
		    closest = dist;
		    close_x = fid_x;
		    close_y = fid_y;
		}
		if (dist < error_threshold) {
		    got_fid = 1;
		    fid_cnt++;
		    if (debug) printf(
			"%s %s at (%f, %f), dist %f, %d samples.\n",
			"Good fiducial for",pt_names[i],
			fid_x,fid_y,dist,n_samples[j]);
		    break;
		}
	    }
	    if (debug) 
		printf("i %d, got_fid %d, fid %f %f, closest %f, close %f %f\n",
		       i,got_fid,fid_x,fid_y,closest,close_x,close_y);

	    if (got_fid) {
		/* Just log repeats for no-fiducial. (Probably lights out.) */
		if (no_fid_msg_count) {
		    fprintf(log_FILE,
			     "%s *** NO FIDUCIAL was repeated %d times.\n",
			     timestamp(), no_fid_msg_count);
		    no_fid_msg_count = 0;
		}

		/* Log every Nth successful check. */
		if (i==0 || !do_log)    /* Only increment the counter once. */
		    do_log = ++checks_skipped >= log_skip;
		if (do_log) {
		    fprintf(log_FILE,
			    "%s %s (%f, %f), distance %f pixels.\n",
			    timestamp(),pt_names[i],fid_x,fid_y,dist);
		    checks_skipped = 0;
		}
	    }
	    else if (closest != FAR) { /* Log all misses. */
		snprintf(msgs[n_msgs],LITTLE_BUFSIZ,
			 "%s *** MISSED %s: closest (%f, %f), distance %f.\n",
			 timestamp(),pt_names[i],close_x,close_y,closest);
		fputs(msgs[n_msgs],log_FILE);
		n_msgs++;
	    }
	}

	/* Don't log no-fiducial at night or on weekends when lights are off. */
	if (op_hrs && fid_cnt == 0 && n_msgs == 0 ) {
	    if (no_fid_msg_count++ == 0){ /* Don't be repetitive. */
		snprintf(msgs[n_msgs],LITTLE_BUFSIZ,
			 "%s *** NO FIDUCIAL VISIBLE.\n",
			 timestamp());
		fputs(msgs[n_msgs],log_FILE);
		n_msgs++;
	    }
	}

	/* Don't raise the alarm unless things have been bad for a while. */
	if (debug) 
	    printf("fid_cnt %d, got_ctr %d, got_chk %d, bad_checks %d\n",
		   fid_cnt, got_ctr, got_chk, bad_checks);
	if (fid_cnt == (got_ctr + got_chk)) {
	    /* Got to have all points present to be good. */
	    bad_checks = 0;
	    strncpy(good_ts_buff,timestamp(),LITTLE_BUFSIZ);
	    good_ts = good_ts_buff;
	}
	else if (op_hrs && n_msgs && ++bad_checks >= alarm_delay) {
	    char mail_buff[LITTLE_BUFSIZ];
	    char *to = (debug?dbg_addr:alarm_addr);
	    snprintf(mail_buff,LITTLE_BUFSIZ,
		     "mail %s -s '[Robocops] Camera %s problem' %s",
		     (0 && debug?"-v":""),cam_num,to);
	    if (debug) printf("%s\n",mail_buff);
	    FILE *mailer = popen(mail_buff,"w");
	    for (i=0; i<n_msgs; i++)
		fputs(msgs[i],mailer);
	    fprintf(mailer, 
		    "\n The last known good timestamp was %s.\n", good_ts);
	    fprintf(mailer, "\n %s %s %s",
		    "If you need to stop the camera checkers, use this command:\n",
		    "    sudo /etc/init.d/camera_checker stop\n",
		    "They run on the computers named 'trash' and 'junk'.\n");
	    int status = pclose(mailer);

	    /* Don't whine constantly, and back off exponentially. */
	    bad_checks = 0;
	    alarm_delay = (int)(alarm_delay * alarm_backoff_exponent);
	    fprintf(log_FILE,
		    "%s ***** Alarm mail sent, status %d, next delay %d.\n",
		    timestamp(),status,alarm_delay);
	}

	sleep(check_secs);
    }
    
    mezz_term(0);

    fprintf(log_FILE,
	    "%s Stopping, pts (%d, %d) (%d, %d), n %d, i %d, e %d, c %d, l %d\n", 
	    timestamp(),
	    pts[CTR_PT][X],pts[CTR_PT][Y],
	    pts[CHK_PT][X],pts[CHK_PT][Y],
	    num_frames,frame_interval, error_threshold, check_secs, log_skip);
}
