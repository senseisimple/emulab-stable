/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _rmcd_path_planning_h
#define _rmcd_path_planning_h

/**
 * @file pathPlanning.h
 *
 * Module focused solely on plotting paths for the robots.  Locating and moving
 * the robot is handled by the master controller.
 */

#include "listNode.h"
#include "mtp.h"

#define PP_MIN_OBSTACLE_CROSS 0.20f

/**
 * Return codes for pp_plot_waypoint.
 */
typedef enum {
    PPC_NO_WAYPOINT,		/*< No obstructions, clear to move. */
    PPC_WAYPOINT,		/*< Path obstructed, need to move around. */
    PPC_BLOCKED,		/*< Path completely blocked, try later. */
    PPC_GOAL_IN_OBSTACLE,	/*< Goal is inside an obstacle. */
} pp_plot_code_t;

typedef enum {
    PPT_CORNERPOINT,
    PPT_DETACHED,
    PPT_INTERNAL,
    PPT_OUTOFBOUNDS,
} pp_point_type_t;

enum {
    PPB_HAS_OBSTACLE,
};

enum {
    PPF_HAS_OBSTACLE = (1L << PPB_HAS_OBSTACLE),
};

/**
 * The path plan for a single robot.  The caller of this module is expected to
 * fill out pp_robot, pp_actual_pos, pp_goal_pos, and pp_speed with their
 * desired values.  This module will then take care of plotting a path around
 * any obstacles.
 */
struct path_plan {
    struct lnMinNode pp_link;		 /*< List node header. (unused) */
    unsigned long pp_flags;		 /*< Holds the PPF flags. */
    struct robot_config *pp_robot;	 /*< The robot this plan is for. */
    struct robot_position pp_actual_pos; /*< Current position. */
    struct robot_position pp_waypoint;	 /*< Current waypoint. */
    struct robot_position pp_goal_pos;	 /*< Current goal. */
    struct obstacle_config pp_obstacle;	 /*< First obstacle in the path. */
    float pp_speed;			 /*< Last set speed for the robot. */
};

/**
 * Plot a new waypoint for the given path.
 *
 * @param pp An initialize path object.
 * @return A code that indicates what action to take to reach the goal.
 */
pp_plot_code_t pp_plot_waypoint(struct path_plan *pp);

/**
 * Global data needed for path planning.
 */
struct path_planning_data {
    /**
     * Array of camera bounds for the area.
     */
    struct box *ppd_bounds;

    /**
     * The length of the ppd_bounds array.
     */
    unsigned int ppd_bounds_len;
    
    /**
     * The maximum distance allowed for a line segment.  XXX This is a hack to
     * keep the robot from going too far off course because of a bad angle from
     * vision and such.
     */
    float ppd_max_distance;
};

extern struct path_planning_data pp_data;

#endif
