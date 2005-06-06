/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file test_path.c
 *
 * Path planning testing tool.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "obstacles.h"
#include "pathPlanning.h"

int debug = 0;

#if defined(SIMPLE_PATH)
#include "simple_path.c"
#elif defined(MULTI_PATH)
#include "multi_path.c"
#else
#error "No bounds/obstacles/robots defined!"
#endif

static void usage(const char *progname)
{
    fprintf(stderr,
	    "Usage: %s [OPTIONS]\n"
	    "\n"
	    "Use the path planner to move a robot from the first position\n"
	    "to the second position and back again.\n"
	    "\n"
	    "Options:\n"
	    "  -h\t\tPrint this message\n"
	    "  -d\t\tIncrease debugging level and do not daemonize\n"
	    "  -i iter\tMaximum number of iterations\n"
	    "  -x float\tFirst X position for the robot\n"
	    "  -y float\tFirst Y position for the robot\n"
	    "  -u float\tSecond X position for the robot\n"
	    "  -v float\tSecond Y position for the robot\n",
	    progname);
}

int main(int argc, char *argv[])
{
    int c, lpc, iterations = 50, retval = EXIT_SUCCESS;
    struct path_plan pp_init, pp;

    loginit(0, NULL);

    ob_init();

    for (lpc = 0; obstacles[lpc].id != 0; lpc++) {
	ob_add_obstacle(&obstacles[lpc]);
    }
    
    pp_data.ppd_bounds = bounds;
    pp_data.ppd_bounds_len = 1;
    pp_data.ppd_max_distance = 10;

    memset(&pp_init, 0, sizeof(pp));
    pp_init.pp_robot = &robots[0];
    pp_init.pp_speed = 0.2;

    while ((c = getopt(argc, argv, "di:x:y:u:v:")) != -1) {
	switch (c) {
	case 'd':
	    debug += 1;
	    break;
	case 'i':
	    sscanf(optarg, "%d", &iterations);
	    break;
	case 'x':
	    sscanf(optarg, "%f", &pp_init.pp_actual_pos.x);
	    break;
	case 'y':
	    sscanf(optarg, "%f", &pp_init.pp_actual_pos.y);
	    break;
	case 'u':
	    sscanf(optarg, "%f", &pp_init.pp_goal_pos.x);
	    break;
	case 'v':
	    sscanf(optarg, "%f", &pp_init.pp_goal_pos.y);
	    break;
	case 'h':
	default:
	    usage(argv[0]);
	    exit(0);
	    break;
	}
    }

    if ((pp_init.pp_actual_pos.x == pp_init.pp_goal_pos.x) &&
	(pp_init.pp_actual_pos.y == pp_init.pp_goal_pos.y)) {
	usage(argv[0]);
	exit(0);
    }

    pp = pp_init;
    for (lpc = 0;
	 (lpc < iterations) &&
	     ((pp.pp_actual_pos.x != pp.pp_goal_pos.x) ||
	      (pp.pp_actual_pos.y != pp.pp_goal_pos.y));
	 lpc++) {
	switch (pp_plot_waypoint(&pp)) {
	case PPC_WAYPOINT:
	case PPC_NO_WAYPOINT:
	    pp.pp_actual_pos = pp.pp_waypoint;
	    break;
	case PPC_BLOCKED:
	case PPC_GOAL_IN_OBSTACLE:
	    lpc = iterations;
	    break;
	}
    }

    info("-reverse-\n");

    pp = pp_init;
    pp.pp_actual_pos = pp_init.pp_goal_pos;
    pp.pp_goal_pos = pp_init.pp_actual_pos;
    for (lpc = 0;
	 (lpc < iterations) &&
	     ((pp.pp_actual_pos.x != pp.pp_goal_pos.x) ||
	      (pp.pp_actual_pos.y != pp.pp_goal_pos.y));
	 lpc++) {
	switch (pp_plot_waypoint(&pp)) {
	case PPC_WAYPOINT:
	case PPC_NO_WAYPOINT:
	    pp.pp_actual_pos = pp.pp_waypoint;
	    break;
	case PPC_BLOCKED:
	case PPC_GOAL_IN_OBSTACLE:
	    lpc = iterations;
	    break;
	}
    }
    
    return retval;
}
