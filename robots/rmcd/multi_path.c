/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file multi_path.c
 *
 * Path planning testing tool that has multiple obstacles.
 *
 * @see test_path.c
 */

/**
 * The bounds of the area to use for planning.
 */
static struct box bounds[] = {
    { 0, 0, 10, 10 },
};

/**
 * The set of obstacles in the area.
 */
static struct obstacle_config obstacles[] = {
    { .id = 1, .xmin = 2.25, .ymin = 4.75, .xmax = 2.75, .ymax = 5.25 },
    { .id = 2, .xmin = 2.50, .ymin = 3.25, .xmax = 3.25, .ymax = 6.75 },
    { .id = 3, .xmin = 7.25, .ymin = 3.25, .xmax = 7.75, .ymax = 6.75 },
    { .id = 0 },
};

/**
 * The set of robots in the area.
 */
static struct robot_config robots[] = {
    { 1, "garcia1" },
};
