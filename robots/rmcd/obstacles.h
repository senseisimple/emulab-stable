/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _obstacles_h
#define _obstacles_h

/**
 * @file obstacles.h
 *
 * Static and dynamic obstacle management.
 */

#include "mtp.h"
#include "rclip.h"
#include "listNode.h"

/**
 * Our guesstimate of the size of an obstacle detected by the robot's sensors.
 * The object is assumed to be square, so the number is the length in meters
 * for one side.
 */
#define DYNAMIC_OBSTACLE_SIZE 0.10f

/**
 * The number of seconds to wait before removing a dynamically created
 * obstacle.
 */
#define OB_DECAY_START 30

/**
 * Our guess as to how far away a sensor contact is from a robot (in meters).
 */
#define OB_CONTACT_DISTANCE 0.15f

/**
 * The size of the longest side of a garcia, used as the natural size of a
 * robot obstacle.
 */
#define ROBOT_OBSTACLE_SIZE 0.28f

/**
 * The type of obstacle.
 */
typedef enum {
    OBT_NONE,
    
    OBT_STATIC,		/*< Obstacle taken from the DB. */
    OBT_DYNAMIC,	/*< Obstacle detected with the proximity sensors. */
    OBT_ROBOT,		/*< Obstacle for a stationary robot. */
} ob_type_t;

/**
 * Structure used to represent static and dynamic obstacles.  The structure
 * maintains two sets of dimensions for the obstacle: the "natural" size, which
 * represents the actual physical object; and the "expanded" size, which covers
 * the natural portion of the obstacle, the exclusion zone, and any dynamically
 * detected "growths".  Any dynamically added obstacles are removed once the
 * on_decay_seconds field hits zero (for static obstacle it is -1, meaning they
 * do not decay).
 *
 * The "id" field of the obstacle config differs between static and dynamic
 * obstacles.  If on_natural.id is -1, the obstacle is dynamic, otherwise it is
 * the static obstacle ID from the database.  For dynamic obstacles, the
 * on_expanded.id field holds a unique negative number to distinguish between
 * them.
 */
struct obstacle_node {
    struct lnMinNode on_link;		/*< List node header. */
    ob_type_t on_type;			/*< The type of obstacle. x*/
    int on_decay_seconds;		/*< Time until reduction/removal. */
    struct obstacle_config on_natural;	/*< Natural size of the obstacle. */
    struct obstacle_config on_expanded;	/*< Natural size + buffer + other. */
};

/**
 * Initialize the obstacle module.  Must be run once before the other functions
 * are called.
 */
void ob_init(void);

/**
 * Dump the current obstacle list to stderr.
 */
void ob_dump_info(void);

/**
 * Add a static obstacle to the tail of the active list.
 *
 * @param oc An initialized obstacle_config specification.  This value will be
 * stored in the on_natural field and the on_expanded field will be initialized
 * with this value plus a buffer size on all sides.
 * @return The obstacle_node created to track the given obstacle.
 */
struct obstacle_node *ob_add_obstacle(struct obstacle_config *oc);

/**
 * Add an obstacle to the head of the active list for a robot at the given
 * position.
 *
 * @param rp The current position of the robot.
 * @param id The integer ID of the robot.
 * @return The obstacle_node created to represent the robot.
 */
struct obstacle_node *ob_add_robot(struct robot_position *rp, int id);

/**
 * Remove a dynamic obstacle from the active list.
 *
 * @param on The obstacle to remove.
 */
void ob_rem_obstacle(struct obstacle_node *on);

/**
 * Find the first obstacle in the active list that intersects the given line.
 *
 * @see obstacle_data.od_active
 *
 * @param rl_inout An initialized line object that specifies the starting and
 * goal positions of the robot.  On return, this value will be set to the two
 * points where the path intersects the obstacle.
 * @param cross_inout Specifies the minimum distance the path must cross the
 * obstacle for it to be considered an intersection.  If the path intersects an
 * obstacle, this is set to the length of the line that goes through the
 * obstacle.
 * @return The first obstacle found to intersect the given line or NULL if none
 * could be found.
 */
struct obstacle_node *ob_find_intersect(rc_line_t rl_inout,
					float *cross_inout);

/**
 * Alternative version of ob_find_intersect that does not take an inout
 * parameter.
 *
 * @see ob_find_intersect
 * @see obstacle_data.od_active
 *
 * @param actual The current position of the robot.
 * @param goal The goal position of the robot.
 * @param distance_out If the path intersects an obstacle, this is set to the
 * distance from the actual position to the intersected obstacle.
 * @param cross_inout Specifies the minimum distance the path must cross the
 * obstacle for it to be considered an intersection.  If the path intersects an
 * obstacle, this is set to the length of the line that goes through the
 * obstacle.
 * @return The first obstacle found to intersect the given line or NULL if none
 * could be found.
 */
struct obstacle_node *ob_find_intersect2(struct robot_position *actual,
					 struct robot_position *goal,
					 float *distance_out,
					 float *cross_inout);

/**
 * Find the first obstacle in the active list that overlaps with the given one.
 * Because the active list is ordered so that dynamic obstacles are at the
 * front, those will be preferred over static obstacles.
 *
 * @see obstacle_data.od_active
 *
 * @param oc An initialized obstacle_config.
 * @return The first obstacle_node found to intersect the given obstacle.
 */
struct obstacle_node *ob_find_overlap(struct obstacle_config *oc);

/**
 * Compute the world coordinates of an obstacle based on a sensor contact given
 * in robot local coordinates.  Because the sensor readings aren't terribly
 * accurate, we compute the compass direction of the point and assume the
 * contact is a fixed distance away.
 *
 * @see OB_CONTACT_DISTANCE
 *
 * @param dst_out The destination for the world coordinates.
 * @param actual The current position of the robot.
 * @param cp_local The contact point in local coordinates.
 */
void ob_obstacle_location(struct contact_point *dst_out,
			  struct robot_position *actual,
			  struct contact_point *cp_local);

void ob_cancel_obstacle(struct robot_position *actual);

/**
 * Construct a dynamic obstacle that was detected by a robot.  If the robot
 * detects an obstacle that overlaps with an existing dynamic obstacle, the
 * preexisting one will be expanded.  Otherwise, a new obstacle is created and
 * added to the list.  After a short period of time, the obstacle list will be
 * restored on the assumption that the detected obstacle was transient.
 *
 * @see ob_tick
 *
 * @param actual The current position of the robot that detected the obstacle.
 * @param cp_local The approximate distance of the detected obstacle from the
 * robot.  The values should be in robot local coordinates, for example a
 * contact point of (0.1,0.0) would be ten centimeters in front of the head of
 * robot.
 * @return A new or existing obstacle_node that encompasses the newly detected
 * obstacle.
 */
struct obstacle_node *ob_found_obstacle(struct robot_position *actual,
					struct contact_point *cp_world);

/**
 * Merge two obstacle dimensions into a single all encompassing one.
 *
 * @param dst_inout The size of the first obstacle and the destination for the
 * merge.
 * @param src The size of the second obstacle.
 */
void ob_merge_obstacles(struct obstacle_config *dst_inout,
			struct obstacle_config *src);

/**
 * Expand the size of an obstacle on all sides.
 *
 * @param dst_out The destination for the expanded size.
 * @param src The original size of the obstacle.
 * @param amount The amount by which to expand the size of the obstacle.
 */
void ob_expand_obstacle(struct obstacle_config *dst_out,
			struct obstacle_config *src,
			float amount);

/**
 * Periodic function that maintains the obstacle list.  Currently, it walks the
 * list of obstacles and reduces the size of any static obstacles that have
 * decayed and removes any dynamic obstacles that have decayed.
 */
void ob_tick(void);

/**
 * Check that the obstacle data is sane.
 *
 * @return true
 */
int ob_data_invariant(void);

/**
 * Global data for the obstacle module.
 */
struct obstacle_data {
    /**
     * The list of currently active obstacles.  The list is ordered so that
     * dynamic obstacles are placed first.  This ordering means that dynamic
     * obstacles will be "preferred" over the statics by the ob_find_intersect
     * and ob_find_overlap functions.  We do this because we want the
     * ob_found_obstacle function to continue to expand a dynamically detected
     * obstacle and not one of the existing static obstacles.  For example, if
     * there is a sensor contact between the dynamic obstacle "a" and the
     * static obstacle "b", we want "a" to be expanded:
     *
     *     a
     *      R
     *     bbbb
     *
     * If we were to expand "b" in this case, the robot's path would end up
     * intersecting both obstacles instead of just "a".
     */
    struct lnMinList od_active;

    /**
     * The list of free obstacle_node objects.
     */
    struct lnMinList od_free_list;

    /**
     * When not NULL, any updates to the obstacle list should be sent through
     * this handle.
     *
     * @see MTP_UPDATE_OBSTACLE
     * @see MTP_REMOVE_OBSTACLE
     */
    mtp_handle_t od_emc_handle;
};

extern struct obstacle_data ob_data;

#endif
