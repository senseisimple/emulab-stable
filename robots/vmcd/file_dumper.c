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

#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"
#include "mtp.h"
#include <math.h>

#include "mezz.h"


static volatile unsigned int mezz_frame_count = 0;
static double x_offset = 0.0;
static double y_offset = 0.0;
static double z_offset = 0.0;

static char *mezz_file,*output_file,*input_file;
static int num_frames = 5;
static int frame_interval = 1;
static FILE *output_FILE;
static FILE *input_FILE;

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
    struct robot_position dp;
    struct robot_position rp;

    dp.x = 0.0;
    dp.y = 0.0;
    dp.theta = 0.0;
    
    rp = *p_inout;

    mtp_polar(&dp, &rp, &distance_from_center, &theta);
    vtheta = atan2f(z_offset, distance_from_center);
    offset = ROBOT_HEIGHT / tanf(vtheta);
    mtp_cartesian(&dp, distance_from_center - offset, theta, p_inout);
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

void print_packets(mezz_mmap_t *mm)
{
    struct robot_position rp;
    mezz_objectlist_t *mol;
    int lpc;
    
    assert(mm != NULL);

    mol = &mm->objectlist;

    fprintf(output_FILE,"frame %d (timestamp %f):\n",
	    mezz_frame_count,mm->time
	    );

    for (lpc = 0; lpc < mol->count; ++lpc) {
        if (mol->objects[lpc].valid) { /* Don't send meaningless data. */

	    /* Copy the camera-coordinates, then */
            rp.x = mol->objects[lpc].px;
            rp.y = mol->objects[lpc].py;
            rp.theta = mol->objects[lpc].pa;

	    fprintf(output_FILE,"  - local[%d]:  x = %f, y = %f, theta = %f\n",
		    lpc,rp.x,rp.y,rp.theta
		    );

	    radial_trans(&rp);

	    fprintf(output_FILE,
		    "  - radial[%d]: x = %f, y = %f, theta = %f\n",
		    lpc,rp.x,rp.y,rp.theta
		    );

	    /* ... transform them into global coordinates. */
            local2global_posit_trans(&rp);

	    fprintf(output_FILE,
		    "  - global[%d]: x = %f, y = %f, theta = %f\n",
		    lpc,rp.x,rp.y,rp.theta
		    );
        }
    }
}

void usage() {

}

int main(int argc, char *argv[])
{
    int c;
    mezz_mmap_t *mezzmap = NULL;
    struct sigaction sa;
    char **default_section_title;
    int default_section_title_len;
    int section_counter = 0;
    int current_default_section_title_len = 64;

    default_section_title = (char **)malloc(sizeof(char *)*
					    current_default_section_title_len);
    if (default_section_title == NULL) {
	error("no memory for section titles!  :-(\n");
	exit(1);
    }
    
    while ((c = getopt(argc, argv, "x:y:z:n:i:f:F:")) != -1) {
        switch (c) {
	case 'f':
	    input_file = optarg;
	    break;
	case 'F':
	    output_file = optarg;
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
    
    if (argc < 1) {
        error("error: missing mezzanine ipc file\n");
        usage();
        exit(1);
    }
    else {
	mezz_file = argv[0];
    }

    signal(SIGPIPE, SIG_IGN);
    
    if (mezz_init(0, mezz_file) == -1) {
	errorc("unable to initialize mezzanine\n");
	exit(2);
    }
    mezzmap = mezz_mmap();
	
    
    unsigned int last_dumped_frame = 0;
    unsigned int frames_needed;
    int i;

    char *section_title = NULL;
    char section_title_buffer[256];

    /* read default section names */
    if (input_file != NULL) {
	input_FILE = fopen(input_file,"r");
	section_counter = 0;

	if (input_FILE != NULL) {
	    while (fgets(section_title_buffer,255,input_FILE) != NULL) {
		if (strlen(section_title_buffer) > 0 && 
		    section_title_buffer[strlen(section_title_buffer)-1] 
		    != '\n'
		    ) {
		    /* we have too much title, truncate the line by reading 
		     * rest of it
		     */
		    int c;
		    while ((c = fgetc(input_FILE)) != EOF && c != '\n')
			;
		}
		
		//printf("herea: %s %d\n",section_title_buffer,
		//       strlen(section_title_buffer));

		// kill the newline, if there was one...
		if (strlen(section_title_buffer) > 0) {
		    //printf("c\n");
		    section_title_buffer[strlen(section_title_buffer)-1] = 
			'\0';
		}


		// if the line is blank or the very first char is a hash, skip
		if (strlen(section_title_buffer) < 1 || 
		    section_title_buffer[0] == '#'
		    ) {
		    continue;
		}

		//printf("hereb\n");

		if (section_counter == default_section_title_len) {
		    // need to realloc
		    
		    if (realloc(default_section_title,
				(default_section_title_len+64)*sizeof(char)
				) == NULL) {
			error("no more memory while reading section"
			      " titles\n");
			exit(1);
		    }
			// null out new pointers...
		    for (i = default_section_title_len; 
			 i < (default_section_title_len + 64);
			 ++i
			 ) {
			default_section_title[i] = NULL;
			}
		    default_section_title_len += 64;
		}

		//printf("herec\n");

		
		default_section_title[section_counter] = (char *)
		    malloc(sizeof(char)*(strlen(section_title_buffer)+1));
		
		if (default_section_title[section_counter] == NULL) {
		    error("error: no memory for section title copy\n");
		    exit(1);
		}

		//printf("hered\n");
		
		strncpy(default_section_title[section_counter],
			section_title_buffer,
			strlen(section_title_buffer)+1
			);
		
		++section_counter;
		//printf("heree\n");
		
	    }
	    printf("Read %d sections.\n",section_counter);
	}

	fclose(input_FILE);
    }

    default_section_title_len = section_counter;
    
    if (output_file != NULL) {
	output_FILE = fopen(output_file,"a");
	if (output_FILE == NULL) {
	    error("error: could not open output file for appending\n");
	    exit(1);
	}
    }
    else {
	output_FILE = stdout;
    }


    fprintf(output_FILE,
	    "Starting file dump:\n"
	    "  - output file: %s\n"
	    "  - input file: %s\n"
	    "  - x_offset: %f\n"
	    "  - y_offset: %f\n"
	    "  - z_offset: %f\n"
	    "  - number of frames: %d\n"
	    "  - frame interval: %d\n"
	    "\n",
	    output_file,input_file,
	    x_offset,y_offset,z_offset,
	    num_frames,frame_interval
	    );


    /*
     * Install our own SIGUSR1 handler _over_ the one installed by mezz_init,
     * so we can increment the local frame count and really be able to tell
     * when a frame was received.
     */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    section_counter = 0;
    section_title_buffer[0] = '\0';

    while (1) { /* The main loop. */
	if (default_section_title_len != 0) {
	    if (section_counter >= default_section_title_len) {
		section_title_buffer[0] = '\0';
		section_title = section_title_buffer;
	    }
	    else {
		section_title_buffer[0] = '\0';
		section_title = default_section_title[section_counter];
		++section_counter;
	    }
	}
	else {
	    section_title = section_title_buffer;
	    section_title_buffer[0] = '\0';
	}

	/* prompt for input... */
	fprintf(stdout,"\nSection title [%s]: ",section_title);
	fflush(stdout);
	fgets(section_title_buffer,255,stdin);

	if (strlen(section_title_buffer) == 0) {
	    ;
	}
	else {
	    section_title_buffer[strlen(section_title_buffer)-1] = '\0';
	}

	if (strlen(section_title_buffer) == 0 && strlen(section_title) == 0) {
	    exit(0);
	}

	/* get frame data */
	/* let signals in... */
	/* bit of a race here, but should be no prob */
	last_dumped_frame = mezz_frame_count;
	frames_needed = num_frames;

	if (strlen(section_title_buffer) > 0) {
	    section_title = section_title_buffer;
	}

	fprintf(output_FILE,
		"+++\n"
		"section: %s\n",
		section_title
		);

	sa.sa_handler = sigusr1;
	sigaction(SIGUSR1, &sa, NULL);

	while (frames_needed > 0) {

	    /* one second should always be plently of time to get
	     * a SIGUSR1
	     */
	    sleep(1);

	    /* disable mezz interrupts */
	    //sa.sa_handler = SIG_IGN;
	    //sigaction(SIGUSR1,&sa,NULL);

	    if (mezz_frame_count >= (last_dumped_frame + frame_interval)) {
		/* new frame */
		if (mezz_frame_count != 
		    (last_dumped_frame + frame_interval)
		    ) {
		    // must have missed one!
		    fprintf(output_FILE,"WARNING: missed %d frames!\n",
			    (mezz_frame_count - (last_dumped_frame +
						 frame_interval))
			    );
		}
		last_dumped_frame = mezz_frame_count;
		--frames_needed;
		print_packets(mezzmap);
	    }

	    /* reenable interrupts */
	    //sa.sa_handler = sigusr1;
	    //sigaction(SIGUSR1,&sa,NULL);

	}

	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1,&sa,NULL);

	fprintf(output_FILE,"+++\n\n");

    }
    

    mezz_term(0);
    
}
