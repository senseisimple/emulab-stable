/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <math.h>
#include <float.h>
#include <assert.h>

#include "log.h"
#include "rmcd.h"
#include "rclip.h"
#include "obstacles.h"
#include "pilotConnection.h"
#include "pathPlanning.h"

#define PP_TOL 0.2

struct path_planning_data pp_data;

/**
 * Classify a position relative to an obstacle.
 *
 * @param rpoint The point to classify.
 * @param oc The obstacle to compare the point against.
 * @return The classification of the point.
 */
static pp_point_type_t pp_point_identify(struct robot_position *rpoint,
					 struct obstacle_config *oc);

/**
 * Compute the next corner point to go to.
 *
 * @param pp The plan to operate on.  The pp_obstacle field should hold the
 * obstacle whose corners should be considered.  On a successful return, the
 * pp_waypoint field will hold the chosen corner point.
 * @return True if a reachable point was found.
 */
static int pp_next_cornerpoint(struct path_plan *pp);

/**
 * @param pt1 The first position.
 * @param pt2 The second position.
 * @return The distance between the two points.
 */
static float pp_point_distance(struct robot_position *pt1,
			       struct robot_position *pt2);

/**
 * Check that a point is in the camera bounds and not in an obstacle.
 *
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return True if the point is valid, false otherwise.
 */
static int pp_point_in_bounds(float x, float y)
{
    int lpc, retval = 0;
    struct rc_line rl;

    for (lpc = 0; lpc < pp_data.ppd_bounds_len; lpc++) {
	struct box *b = &pp_data.ppd_bounds[lpc];
	
	if (x >= b->x && y >= b->y &&
	    x < (b->x + b->width) &&
	    y < (b->y + b->height)) {
	    retval = 1;
	    break;
	}
    }

    if (retval) {
	rl.x0 = rl.x1 = x;
	rl.y0 = rl.y1 = y;
	if (ob_find_intersect(&rl, NULL) != NULL)
	    retval = 0;
    }
    
    return retval;
}

/**
 * Check that a point is not only in bounds, but also reachable by the robot.
 * A point is not reachable if it is out of bounds or if the robot would have
 * to travel through the center of the obstacle to reach it.
 *
 * @param pp A plan with the pp_actual_pos and pp_obstacle fields initialized.
 * The pp_obstacle field should contain the obstacle that must be navigated
 * around.
 * @param rp The point to check.
 * @return True if the point is reachable, false otherwise.
 */
static int pp_point_reachable(struct path_plan *pp, struct robot_position *rp)
{
    int retval = 0;

    assert(pp != NULL);
    assert(rp != NULL);

    if (pp_point_in_bounds(rp->x, rp->y)) {
	struct obstacle_config oc;
	struct rc_line rl;

	rl.x0 = pp->pp_actual_pos.x;
	rl.y0 = pp->pp_actual_pos.y;
	rl.x1 = rp->x;
	rl.y1 = rp->y;
	oc = pp->pp_obstacle;
	if (rc_compute_code(rl.x0, rl.y0, &oc) == 0) {
	    float xlen, ylen;

	    /*
	     * The point is inside the obstacle, shrink the size of the
	     * obstacle so the point is on the outside.
	     */
	    xlen = min(fabs(rl.x0 - pp->pp_obstacle.xmin),
		       fabs(rl.x0 - pp->pp_obstacle.xmax));
	    ylen = min(fabs(rl.y0 - pp->pp_obstacle.ymin),
		       fabs(rl.y0 - pp->pp_obstacle.ymax));
	    if (xlen < ylen) {
		if (fabs(rl.x0 - pp->pp_obstacle.xmin) <
		    fabs(rl.x0 - pp->pp_obstacle.xmax)) {
		    oc.xmin = rl.x0 + 0.01;
		    oc.xmax = pp->pp_obstacle.xmax;
		}
		else {
		    oc.xmin = pp->pp_obstacle.xmin;
		    oc.xmax = rl.x0 - 0.01;
		}
	    }
	    else {
		if (fabs(rl.y0 - pp->pp_obstacle.ymin) <
		    fabs(rl.y0 - pp->pp_obstacle.ymax)) {
		    oc.ymin = rl.y0 + 0.01;
		    oc.ymax = pp->pp_obstacle.ymax;
		}
		else {
		    oc.ymin = pp->pp_obstacle.ymin;
		    oc.ymax = rl.y0 - 0.01;
		}
	    }
	}
	
	if (debug > 1) {
	    info("reachable %.2f %.2f - %.2f %.2f %.2f %.2f\n",
		 rp->x, rp->y, oc.xmin, oc.ymin, oc.xmax, oc.ymax);
	}

	/*
	 * Check if the path to the point is completely outside the obstacle
	 * or intersects just at one of the corners.
	 */
	if ((rc_clip_line(&rl, &oc) == 0) ||
	    ((rl.x0 == rl.x1) && (rl.y0 == rl.y1))) {
	    retval = 1;
	}
    }

    return retval;
}

pp_plot_code_t pp_plot_waypoint(struct path_plan *pp)
{
    struct lnMinList dextra, sextra, intersections;
    float distance, cross, theta;
    pp_plot_code_t retval;
    int in_ob = 0;
    
    assert(pp != NULL);

    if (debug > 0) {
	info(" plot %.2f %.2f -> %.2f %.2f\n",
	     pp->pp_actual_pos.x, pp->pp_actual_pos.y,
	     pp->pp_goal_pos.x, pp->pp_goal_pos.y);
    }
    
    lnNewList(&dextra);
    lnNewList(&sextra);
    lnNewList(&intersections);
    
    retval = PPC_NO_WAYPOINT; /* if nothing else */
    
    /*
     * We have to iterate over one or more waypoints, so we start with the
     * Initial waypoint is the goal point.
     */
    pp->pp_waypoint = pp->pp_goal_pos;
    
    do {
	struct obstacle_node *on, *min_on = NULL;
	float min_distance = FLT_MAX;
	int robot_ob = 0;

	/*
	 * The path doesn't cross enough of the obstacle box for us to worry
	 * about it so we dump it on the extra list so the intersect doesn't
	 * return it again.  XXX The number is partially a guess and
	 * empirically derived.
	 */
	cross = PP_MIN_OBSTACLE_CROSS;
	/*
	 * First we find all of the obstacles that intersect with this path
	 * and locate the minimum distance to an obstacle.
	 */
	if (debug)
	    info("wp %.2f %.2f\n", pp->pp_waypoint.x, pp->pp_waypoint.y);
	while ((on = ob_find_intersect2(&pp->pp_actual_pos,
					&pp->pp_waypoint,
					&distance,
					&cross)) != NULL) {
#if 0
	    info("intr %d %.2f %.2f\n",
		 on->on_natural.id,
		 distance,
		 cross);
#endif
	    
	    if (on->on_type == OBT_ROBOT)
		robot_ob = 1;

	    switch (pp_point_identify(&pp->pp_actual_pos, &on->on_expanded)) {
	    case PPT_INTERNAL:
	    case PPT_CORNERPOINT:
	    case PPT_OUTOFBOUNDS:
		in_ob = 1;
		break;
	    default:
		break;
	    }
	    
	    lnRemove(&on->on_link);
	    /* Record the intersection and */
	    lnAddTail(&intersections, &on->on_link);
	    /* ... check if this is the closest obstacle. */
	    if (distance < min_distance) {
		min_distance = distance;
		min_on = on;
	    }
	    
	    cross = PP_MIN_OBSTACLE_CROSS;
	}
	
	/* Check if there was an actual intersection. */
	if (min_on != NULL) {
	    if (debug > 1)
		info("closest %d\n", min_on->on_natural.id);
	    
	    pp->pp_flags |= PPF_HAS_OBSTACLE;
	    lnRemove(&min_on->on_link); // from the intersections list
	    lnAddTail(min_on->on_type == OBT_STATIC ? &sextra : &dextra,
		      &min_on->on_link);
	    pp->pp_obstacle = min_on->on_expanded;

	    /*
	     * Find all obstacles that the path intersects with and overlap the
	     * closest one.  The rest of the obstacles will be dealt with after
	     * we get around the closest one.
	     */
	    while ((on = (struct obstacle_node *)
		    lnRemHead(&intersections)) != NULL) {
		if (rc_rect_intersects(&pp->pp_obstacle, &on->on_expanded)) {
		    if (debug > 1)
			info("merging %d\n", on->on_natural.id);
		    
		    ob_merge_obstacles(&pp->pp_obstacle, &on->on_expanded);
		}
		lnAddTail(on->on_type == OBT_STATIC ? &sextra : &dextra,
			  &on->on_link);
	    }
	    
	    if (rc_compute_code(pp->pp_goal_pos.x,
				pp->pp_goal_pos.y,
				&pp->pp_obstacle) == 0) {
		struct robot_position rp;
		struct rc_line rl;
		float r;
		
		mtp_polar(&pp->pp_goal_pos, &pp->pp_actual_pos, &r, &theta);
		if (robot_ob)
		    theta += M_PI_2;
		mtp_cartesian(&pp->pp_goal_pos, r + 1000.0, theta, &rp);
		rl.x0 = pp->pp_goal_pos.x;
		rl.y0 = pp->pp_goal_pos.y;
		rl.x1 = rp.x;
		rl.y1 = rp.y;
		rc_clip_line(&rl, &pp->pp_obstacle);
		pp->pp_waypoint.x = rl.x1;
		pp->pp_waypoint.y = rl.y1;
		if (pp_point_distance(&pp->pp_waypoint,
				      &pp->pp_actual_pos) > 0.075) {
		    if (pp_point_reachable(pp, &pp->pp_waypoint))
			retval = PPC_WAYPOINT;
		    else if (pp_next_cornerpoint(pp))
			retval = PPC_WAYPOINT;
		    else
			retval = PPC_BLOCKED;
		}
		else {
		    retval = PPC_GOAL_IN_OBSTACLE;
		}
	    }
	    else if (pp_next_cornerpoint(pp)) {
		retval = PPC_WAYPOINT;
	    }
	    else {
		retval = PPC_BLOCKED;
	    }
	}

	/* Restore the active obstacle list.  XXX Shouldn't do this here. */
	lnPrependList(&ob_data.od_active, &dextra); // dynamics first
	lnAppendList(&ob_data.od_active, &sextra); // statics last
	
	/* Check for obstacles on the path to our new waypoint. */
	cross = PP_MIN_OBSTACLE_CROSS;
    } while ((retval == PPC_WAYPOINT) &&
	     (ob_find_intersect2(&pp->pp_actual_pos,
				 &pp->pp_waypoint,
				 &distance,
				 &cross) != NULL) &&
	     !in_ob);
    
    /* And make sure it is still sane. */
    assert(ob_data_invariant());
    
    /* restrict final waypoint to MAX_DISTANCE */
    mtp_polar(&pp->pp_actual_pos,
	      (retval == PPC_WAYPOINT) ? &pp->pp_waypoint : &pp->pp_goal_pos,
	      &distance,
	      &theta);
    if (distance > pp_data.ppd_max_distance) {
	mtp_cartesian(&pp->pp_actual_pos,
		      pp_data.ppd_max_distance,
		      theta,
		      &pp->pp_waypoint);
	retval = PPC_WAYPOINT;
    }

    switch (retval) {
    case PPC_WAYPOINT:
	info("set waypoint(%s) %.2f %.2f\n",
	     pp->pp_robot->hostname,
	     pp->pp_waypoint.x,
	     pp->pp_waypoint.y);
	break;
    case PPC_NO_WAYPOINT:
	info("set waypoint(%s) NONE\n", pp->pp_robot->hostname);
	break;
    case PPC_BLOCKED:
	info("set waypoint(%s) BLOCKED\n", pp->pp_robot->hostname);
	break;
    case PPC_GOAL_IN_OBSTACLE:
	info("set waypoint(%s) GOAL_IN_OBSTACLE\n", pp->pp_robot->hostname);
	break;
    }
    
    return retval;
}

pp_point_type_t pp_point_identify(struct robot_position *rpoint,
                                  struct obstacle_config *oc)
{
    /* determine the point type of rpoint */
    
    pp_point_type_t retval;
    robot_position this_cp;

    assert(rpoint != NULL);
    assert(oc != NULL);

    if (rc_compute_code(rpoint->x, rpoint->y, oc) == 0)
	retval = PPT_INTERNAL;
    else
	retval = PPT_DETACHED;
    
    /* is this a corner point? Check each corner of this obstacle */
    if (PP_TOL > pp_point_distance(
	rpoint, rc_corner(RCF_TOP|RCF_LEFT, &this_cp, oc))) {
	retval = PPT_CORNERPOINT;
    }
    else if (PP_TOL > pp_point_distance(
	rpoint, rc_corner(RCF_TOP|RCF_RIGHT, &this_cp, oc))) {
	retval = PPT_CORNERPOINT;
    }
    else if (PP_TOL > pp_point_distance(
	rpoint, rc_corner(RCF_BOTTOM|RCF_RIGHT, &this_cp, oc))) {
	retval = PPT_CORNERPOINT;
    }
    else if (PP_TOL > pp_point_distance(
	rpoint, rc_corner(RCF_BOTTOM|RCF_LEFT, &this_cp, oc))) {
	retval = PPT_CORNERPOINT;
    }
    
    /* is this point in bounds? */
    if (!(pp_point_in_bounds(rpoint->x, rpoint->y))) {
	retval = PPT_OUTOFBOUNDS;
	if (debug > 1)
	    info("pp_point_identify says this point is out of bounds\n");
    }
    
    /* debugging output */
    if (PPT_CORNERPOINT == retval) {
	if (debug > 1)
	    info("pp_point_identify says this point is a corner point\n");
    }

    return retval;
}

int pp_next_cornerpoint(struct path_plan *pp)
{
    int retval = 0;

    float bl_dist, br_dist, tl_dist, tr_dist;
    float bl_distg, br_distg, tl_distg, tr_distg;
    float min_dist = 0;

    robot_position cp_bl; /* bottom left corner point */
    robot_position cp_tl; /* top left corner point */
    robot_position cp_br; /* bottom right corner point */
    robot_position cp_tr; /* top right corner point */

    assert(pp != NULL);
    
    /* corner points of this obstacle */
    /* SWAP ymin/ymax because of y coord system is AFU */
    cp_bl.x = pp->pp_obstacle.xmin;
    cp_bl.y = pp->pp_obstacle.ymax;
    
    cp_tl.x = pp->pp_obstacle.xmin;
    cp_tl.y = pp->pp_obstacle.ymin;
    
    cp_br.x = pp->pp_obstacle.xmax;
    cp_br.y = pp->pp_obstacle.ymax;
    
    cp_tr.x = pp->pp_obstacle.xmax;
    cp_tr.y = pp->pp_obstacle.ymin;
    
    /* get the distances to each CP from the IP, and FP */
    bl_dist = pp_point_distance(&pp->pp_actual_pos, &cp_bl);
    bl_distg = pp_point_distance(&pp->pp_goal_pos, &cp_bl);
    
    tl_dist = pp_point_distance(&pp->pp_actual_pos, &cp_tl);
    tl_distg = pp_point_distance(&pp->pp_goal_pos, &cp_tl);
    
    br_dist = pp_point_distance(&pp->pp_actual_pos, &cp_br);
    br_distg = pp_point_distance(&pp->pp_goal_pos, &cp_br);
    
    tr_dist = pp_point_distance(&pp->pp_actual_pos, &cp_tr);
    tr_distg = pp_point_distance(&pp->pp_goal_pos, &cp_tr);
    
    switch (pp_point_identify(&pp->pp_actual_pos, &pp->pp_obstacle)) {
	
    case PPT_CORNERPOINT:
	/* actual point is already a corner point */
	/* proceed to the next CP nearest the FP */

	if (PP_TOL > bl_dist || PP_TOL > tr_dist) {
	    /* This is the bottom left or top right corner point */
	    /* the next corner point is BR or TL, find the closest */

	    if (debug > 1) {
		info("pp_next_corner_point: currently at bottom-left or "
		     "top-right, going to next corner point.\n");
	    }
	    
	    if (!pp_point_in_bounds(cp_br.x, cp_br.y) &&
		!pp_point_in_bounds(cp_tl.x, cp_tl.y)) {
		/* neither CP is in bounds -- CAN'T GO ANYWHERE */
		printf("pp_next_corner_point: No corner points are in bounds\n");
	    }
	    
	    if (pp_point_in_bounds(cp_br.x, cp_br.y) &&
		!pp_point_in_bounds(cp_tl.x, cp_tl.y)) {
		/* bottom right CP is in bounds, but top left is not */
		pp->pp_waypoint = cp_br; /* must go to bottom right */
		if (debug > 1)
		    info("pp_next_corner_point: must go to bottom right\n");
		retval = 1;
	    }
	    
	    if (!pp_point_in_bounds(cp_br.x, cp_br.y) &&
		pp_point_in_bounds(cp_tl.x, cp_tl.y)) {
		/* top left CP is in bounds, but bottom right is not */
		pp->pp_waypoint = cp_tl; /* must go to top left */
		if (debug > 1)
		    info("pp_next_corner_point: must go to top left\n");
		retval = 1;
	    }
	    
	    if (pp_point_in_bounds(cp_br.x, cp_br.y) &&
		pp_point_in_bounds(cp_tl.x, cp_tl.y)) {
		/* both CPs are in bounds
		 * go to the closest CP to the FP
		 */
		
		if (br_distg < tl_distg) {
		    /* bottom right is closer */
		    pp->pp_waypoint = cp_br;
		    if (debug > 1)
			info("pp_next_corner_point: going to bottom right\n");
		    retval = 1;
		}
		else {
		    /* top left is closer, or EQUAL */
		    pp->pp_waypoint = cp_tl;
		    if (debug > 1)
			info("pp_next_corner_point: going to top left\n");
		    retval = 1;
		}
	    }  
	}
	else {
	    /* This is the top left or bottom right corner point */
	    /* the next corner point is BL or TR, find the closest */

	    if (debug > 1) {
		info("pp_next_corner_point: currently at top left or bottom right, going to next corner point.\n");
	    }
	    
	    if (!pp_point_in_bounds(cp_bl.x, cp_bl.y) &&
		!pp_point_in_bounds(cp_tr.x, cp_tr.y)) {
		/* neither CP is in bounds -- CAN'T GO ANYWHERE */
		printf("pp_next_corner_point: No corner points are in bounds\n");
	    }
	    
	    if (pp_point_in_bounds(cp_bl.x, cp_bl.y) &&
		!pp_point_in_bounds(cp_tr.x, cp_tr.y)) {
		/* bottom left CP is in bounds, but top right is not */
		pp->pp_waypoint = cp_bl; /* must go to bottom left */
		if (debug > 1)
		    info("pp_next_corner_point: must go to bottom left\n");
		retval = 1;
	    }
	    
	    if (!pp_point_in_bounds(cp_bl.x, cp_bl.y) &&
		pp_point_in_bounds(cp_tr.x, cp_tr.y)) {
		/* top right CP is in bounds, but bottom left is not */
		pp->pp_waypoint = cp_tr; /* must go to top right */
		if (debug > 1)
		    info("pp_next_corner_point: must go to top right\n");
		retval = 1;
	    }
	    
	    if (pp_point_in_bounds(cp_bl.x, cp_bl.y) &&
		pp_point_in_bounds(cp_tr.x, cp_tr.y)) {
		/* both CPs are in bounds
		 * go to the closest CP to the FP
		 */
		
		if (bl_distg < tr_distg) {
		    /* bottom left is closer */
		    pp->pp_waypoint = cp_bl;
		    if (debug > 1)
			info("pp_next_corner_point: going to bottom left\n");
		    retval = 1;
		}
		else {
		    /* top right is closer, or EQUAL */
		    pp->pp_waypoint = cp_tr;
		    if (debug > 1)
			info("pp_next_corner_point: going to top right\n");
		    retval = 1;
		}
	    }
	}
	break;

    case PPT_DETACHED:
    case PPT_INTERNAL:
    case PPT_OUTOFBOUNDS:
	/* actual point is detached from obstacle boundary */
	/* proceed to the CP nearest the IP */
	/* CHANGE HERE TO ADD OPTIMIZATION */

	if (debug > 1) {
	    info("pp_next_corner_point: not currently at a corner point, going to nearest corner point in bounds.\n");
	}

	bl_dist += bl_distg;
	tl_dist += tl_distg;
	br_dist += br_distg;
	tr_dist += tr_distg;

	if (pp_point_reachable(pp, &cp_bl)) {
	    min_dist = bl_dist;
	    pp->pp_waypoint = cp_bl;
	    retval = 1;
	} else if (pp_point_reachable(pp, &cp_br)) {
	    min_dist = br_dist;
	    pp->pp_waypoint = cp_br;
	    retval = 1;
	} else if (pp_point_reachable(pp, &cp_tl)) {
	    min_dist = tl_dist;
	    pp->pp_waypoint = cp_tl;
	    retval = 1;
	} else if (pp_point_reachable(pp, &cp_tr)) {
	    min_dist = tr_dist;
	    pp->pp_waypoint = cp_tr;
	    retval = 1;
	} else {
	    /* no corner points are in bounds!! */
	    info("pp_next_corner_point: All corner points for this obstacle are out of bounds!\n");
	}
	
	/* find the minimum
	 * (don't need to look at bottom left
	 */
	
	if (br_dist < min_dist && pp_point_reachable(pp, &cp_br)) {
	    min_dist = br_dist;
	    pp->pp_waypoint = cp_br;
	}
	
	if (tl_dist < min_dist && pp_point_reachable(pp, &cp_tl)) {
	    min_dist = tl_dist;
	    pp->pp_waypoint = cp_tl;
	}
	
	if (tr_dist < min_dist && pp_point_reachable(pp, &cp_tr)) {
	    min_dist = tr_dist;
	    pp->pp_waypoint = cp_tr;
	}
	break;
    }

    return retval;
}

float pp_point_distance(struct robot_position *pt1,
			struct robot_position *pt2)
{
    assert(pt1 != NULL);
    assert(pt2 != NULL);
    
    /* simple radial distance between two points */
    return (hypotf(pt1->x - pt2->x, pt1->y - pt2->y));
}
