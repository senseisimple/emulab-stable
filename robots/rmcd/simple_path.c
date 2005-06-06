/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file simple_path.c
 *
 * Path planning testing tool that has a single obstacle.
 */

/**
 * The bounds of the area to use for planning.
 */
static struct box bounds[] = {
    { 0, 0, 10, 10 },
};

/**
 * The obstacle to avoid.
 */
static struct obstacle_config obstacles[] = {
    { .id = 1, .xmin = 4, .ymin = 4, .xmax = 6, .ymax = 6 },
    { .id = 0 },
};

/**
 * The set of robots in the area.
 */
static struct robot_config robots[] = {
    { 1, "garcia1" },
};
