/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <assert.h>

#include "log.h"
#include "rclip.h"
#include "obstacles.h"
#include "pilotConnection.h"

#define OB_FREE_NODE_COUNT 256

struct obstacle_data ob_data;

/**
 * Static allocation of obstacle_node objects.
 *
 * XXX This is terrible.
 */
static struct obstacle_node ob_nodes[OB_FREE_NODE_COUNT];

int ob_data_invariant(void)
{
    struct obstacle_node *on;

    on = (struct obstacle_node *)ob_data.od_active.lh_Head;
    
    /* The first part of the list should be dynamic obstacles, followed by */
    while ((on->on_link.ln_Succ != NULL) &&
	   ((on->on_type == OBT_DYNAMIC) || (on->on_type == OBT_ROBOT))) {
	assert(mtp_obstacle_config_invariant(&on->on_expanded));
	
	on = (struct obstacle_node *)on->on_link.ln_Succ;
    }

    /* ... the static obstacles. */
    while (on->on_link.ln_Succ != NULL) {
	assert(on->on_type == OBT_STATIC); // no dynamics
	assert(mtp_obstacle_config_invariant(&on->on_natural));
	assert(mtp_obstacle_config_invariant(&on->on_expanded));
	
	on = (struct obstacle_node *)on->on_link.ln_Succ;
    }
    
    return 1;
}

void ob_init(void)
{
    static int init_done = 0;
    
    int lpc;

    assert(!init_done);
    
    lnNewList(&ob_data.od_active);
    lnNewList(&ob_data.od_free_list);

    for (lpc = 0; lpc < OB_FREE_NODE_COUNT; lpc++) {
	struct obstacle_node *on = &ob_nodes[lpc];

	on->on_natural.id = -1;
	on->on_expanded.id = -lpc; // Unique ID for dynamic obstacles.
	lnAddTail(&ob_data.od_free_list, &on->on_link);
    }

    init_done = 1;
}

void ob_dump_info(void)
{
    struct obstacle_node *on;

    info("info: obstacle list\n");
    
    on = (struct obstacle_node *)ob_data.od_active.lh_Head;
    while (on->on_link.ln_Succ != NULL) {
	info("  %d(%d): %.2f %.2f %.2f %.2f\t%d\n",
	     on->on_natural.id,
	     on->on_expanded.id,
	     on->on_expanded.xmin,
	     on->on_expanded.ymin,
	     on->on_expanded.xmax,
	     on->on_expanded.ymax,
	     on->on_decay_seconds);
	on = (struct obstacle_node *)on->on_link.ln_Succ;
    }
}

struct obstacle_node *ob_add_obstacle(struct obstacle_config *oc)
{
    struct obstacle_node *retval;
    
    assert(oc != NULL);
    assert(mtp_obstacle_config_invariant(oc));

    retval = (struct obstacle_node *)lnRemHead(&ob_data.od_free_list);
    assert(retval != NULL); // XXX

    retval->on_type = OBT_STATIC;
    retval->on_decay_seconds = -1; // never decay
    retval->on_natural = *oc;
    retval->on_expanded.id = oc->id;
    ob_expand_obstacle(&retval->on_expanded, oc, OBSTACLE_BUFFER);

    lnAddTail(&ob_data.od_active, &retval->on_link); // put statics at the back

    assert(ob_data_invariant());
    
    return retval;
}

struct obstacle_node *ob_add_robot(struct robot_position *rp, int id)
{
    struct obstacle_node *retval;
    
    assert(rp != NULL);

    retval = (struct obstacle_node *)lnRemHead(&ob_data.od_free_list);
    assert(retval != NULL); // XXX

    retval->on_type = OBT_ROBOT;
    retval->on_decay_seconds = -1; // never decay
    /*
     * XXX We don't compute the actual bounding box, we just construct a square
     * with the longest side of the robot.
     */
    retval->on_natural.xmin = rp->x - ROBOT_OBSTACLE_SIZE / 2;
    retval->on_natural.ymin = rp->y - ROBOT_OBSTACLE_SIZE / 2;
    retval->on_natural.xmax = rp->x + ROBOT_OBSTACLE_SIZE / 2;
    retval->on_natural.ymax = rp->y + ROBOT_OBSTACLE_SIZE / 2;
    ob_expand_obstacle(&retval->on_expanded,
		       &retval->on_natural,
		       OBSTACLE_BUFFER);
    
    lnAddHead(&ob_data.od_active, &retval->on_link);

    if (ob_data.od_emc_handle != NULL) {
	mtp_send_packet2(ob_data.od_emc_handle,
			 MA_Opcode, MTP_CREATE_OBSTACLE,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, id,
			 MA_ObstacleVal, &retval->on_expanded,
			 MA_TAG_DONE);
    }

    assert(ob_data_invariant());
    
    return retval;
}

void ob_rem_obstacle(struct obstacle_node *on)
{
    if (on != NULL) {
	assert(on->on_type != OBT_STATIC);
	
	lnRemove(&on->on_link);
	lnAddHead(&ob_data.od_free_list, &on->on_link);
	if (ob_data.od_emc_handle != NULL) {
	    mtp_send_packet2(ob_data.od_emc_handle,
			     MA_Opcode, MTP_REMOVE_OBSTACLE,
			     MA_Role, MTP_ROLE_RMC,
			     MA_ObstacleVal, &on->on_expanded,
			     MA_TAG_DONE);
	}
	
	assert(ob_data_invariant());
    }
}

struct obstacle_node *ob_find_intersect(rc_line_t rl_inout, float *cross_inout)
{
    struct obstacle_node *on, *retval = NULL;
    float _cross = 0.0f;
    
    assert(rl_inout != NULL);

    if (cross_inout == NULL)
	cross_inout = &_cross;
    
    on = (struct obstacle_node *)ob_data.od_active.lh_Head;
    while ((retval == NULL) && (on->on_link.ln_Succ != NULL)) {
	struct rc_line rl = *rl_inout;
	
	if (rc_clip_line(&rl, &on->on_expanded)) {
	    float cross = hypotf(rl.x0 - rl.x1, rl.y0 - rl.y1);
	    
	    if (cross >= *cross_inout) {
		*cross_inout = cross;
		*rl_inout = rl;
		retval = on;
	    }
	}
	on = (struct obstacle_node *)on->on_link.ln_Succ;
    }
    
    return retval;
}

struct obstacle_node *ob_find_intersect2(struct robot_position *actual,
					 struct robot_position *goal,
					 float *distance_out,
					 float *cross_inout)
{
    struct obstacle_node *retval;
    struct rc_line rl;
    
    assert(actual != NULL);
    assert(goal != NULL);
    assert(distance_out != NULL);
    assert(cross_inout != NULL);
    
    rl.x0 = actual->x;
    rl.y0 = actual->y;
    rl.x1 = goal->x;
    rl.y1 = goal->y;

    *distance_out = FLT_MAX; // "infinity"
    if ((retval = ob_find_intersect(&rl, cross_inout)) != NULL)
	*distance_out = hypotf(actual->x - rl.x0, actual->y - rl.y0);
    
    return retval;
}

struct obstacle_node *ob_find_overlap(struct obstacle_config *oc)
{
    struct obstacle_node *on, *retval = NULL;

    assert(oc != NULL);
    assert(mtp_obstacle_config_invariant(oc));

    on = (struct obstacle_node *)ob_data.od_active.lh_Head;
    while ((retval == NULL) && (on->on_link.ln_Succ != NULL)) {
	if (rc_rect_intersects(oc, &on->on_expanded))
	    retval = on;
	
	on = (struct obstacle_node *)on->on_link.ln_Succ;
    }
    
    return retval;
}

void ob_obstacle_location(struct contact_point *dst,
			  struct robot_position *actual,
			  struct contact_point *cp_local)
{
    float local_bearing;
    
    assert(dst != NULL);
    assert(actual != NULL);
    assert(cp_local != NULL);
    
    local_bearing = atan2f(cp_local->y, cp_local->x);
    
    switch (mtp_compass(local_bearing)) {
    case MCF_EAST:
    case MCF_NORTH|MCF_EAST:
    case MCF_SOUTH|MCF_EAST:
	cp_local->x = OB_CONTACT_DISTANCE;
	cp_local->y = 0.0;
	break;

    case MCF_NORTH:
	cp_local->y = OB_CONTACT_DISTANCE;
	break;
    case MCF_SOUTH:
	cp_local->y = -OB_CONTACT_DISTANCE;
	break;
	
    case MCF_WEST:
    case MCF_NORTH|MCF_WEST:
    case MCF_SOUTH|MCF_WEST:
	cp_local->x = -OB_CONTACT_DISTANCE;
	cp_local->y = 0.0;
	break;

    default:
	assert(0);
	break;
    }
    
    REL2ABS(dst, actual->theta, cp_local, actual);
}

struct obstacle_node *ob_found_obstacle(struct robot_position *actual,
					struct contact_point *cp_world)
{
    struct obstacle_node *retval;
    struct obstacle_config oc;
    mtp_opcode_t opcode;
    
    assert(actual != NULL);
    assert(cp_world != NULL);
    
    oc.id = 0; // XXX
    
    oc.xmin = cp_world->x;
    oc.xmax = cp_world->x;
    oc.ymin = cp_world->y;
    oc.ymax = cp_world->y;

    ob_expand_obstacle(&oc, &oc, DYNAMIC_OBSTACLE_SIZE + OBSTACLE_BUFFER);
    
    if (((retval = ob_find_overlap(&oc)) != NULL) &&
	(retval->on_type == OBT_DYNAMIC)) {
	opcode = MTP_UPDATE_OBSTACLE;
	retval->on_decay_seconds = OB_DECAY_START;
	ob_merge_obstacles(&retval->on_expanded, &oc);
	
	info("expanding existing obstacle: %d %.2f %.2f %.2f %.2f\n"
	     " --> %.2f %.2f %.2f %.2f\n",
	     retval->on_natural.id,
	     retval->on_natural.xmin, retval->on_natural.ymin,
	     retval->on_natural.xmax, retval->on_natural.ymax,
	     retval->on_expanded.xmin, retval->on_expanded.ymin,
	     retval->on_expanded.xmax, retval->on_expanded.ymax);
    }
    else {
	info("creating new obstacle -> %.2f %.2f %.2f %.2f\n",
	     oc.xmin, oc.ymin, oc.xmax, oc.ymax);

	opcode = MTP_CREATE_OBSTACLE;
	retval = (struct obstacle_node *)lnRemHead(&ob_data.od_free_list);
	assert(retval != NULL); // XXX

	retval->on_type = OBT_DYNAMIC;
	retval->on_decay_seconds = OB_DECAY_START;
	retval->on_expanded.xmin = oc.xmin;
	retval->on_expanded.ymin = oc.ymin;
	retval->on_expanded.xmax = oc.xmax;
	retval->on_expanded.ymax = oc.ymax;

	/* Put dynamics at the front of the list. */
	lnAddHead(&ob_data.od_active, &retval->on_link);
    }

    // XXX not the best place to do this...
    if (ob_data.od_emc_handle != NULL) {
	mtp_send_packet2(ob_data.od_emc_handle,
			 MA_Opcode, opcode,
			 MA_Role, MTP_ROLE_RMC,
			 MA_ObstacleVal, &retval->on_expanded,
			 MA_TAG_DONE);
    }
    
    return retval;
}

void ob_merge_obstacles(struct obstacle_config *dst,
			struct obstacle_config *src)
{
    assert(dst != NULL);
    assert(mtp_obstacle_config_invariant(dst));
    assert(src != NULL);
    assert(mtp_obstacle_config_invariant(src));
    
    dst->xmin = min(src->xmin, dst->xmin);
    dst->ymin = min(src->ymin, dst->ymin);
    dst->xmax = max(src->xmax, dst->xmax);
    dst->ymax = max(src->ymax, dst->ymax);
    
    assert(mtp_obstacle_config_invariant(dst));
}

void ob_expand_obstacle(struct obstacle_config *dst,
			struct obstacle_config *src,
			float amount)
{
    assert(dst != NULL);
    assert(src != NULL);
    assert(mtp_obstacle_config_invariant(src));
    assert(amount > 0.0f);

    dst->xmin = src->xmin - amount;
    dst->ymin = src->ymin - amount;
    dst->xmax = src->xmax + amount;
    dst->ymax = src->ymax + amount;
    
    assert(mtp_obstacle_config_invariant(dst));
}

void ob_tick(void)
{
    struct obstacle_node *on, *on_succ;

    ob_data_invariant();

    on = (struct obstacle_node *)ob_data.od_active.lh_Head;
    while (on->on_link.ln_Succ != NULL) {
	on_succ = (struct obstacle_node *)on->on_link.ln_Succ;

	if (on->on_decay_seconds >= 0) {
	    on->on_decay_seconds -= 1;
	    if (on->on_decay_seconds == 0) {
		mtp_opcode_t opcode = 0;
		
		switch (on->on_type) {
		case OBT_ROBOT:
		    /* Robot obstacle, reset the size. */
		    ob_expand_obstacle(&on->on_expanded,
				       &on->on_natural,
				       OBSTACLE_BUFFER);
		    opcode = MTP_UPDATE_OBSTACLE;
		    break;
		case OBT_DYNAMIC:
		    /* Dynamic obstacle, remove it completely. */
		    lnRemove(&on->on_link);
		    lnAddHead(&ob_data.od_free_list, &on->on_link);
		    opcode = MTP_REMOVE_OBSTACLE;
		    break;
		default:
		    assert(0);
		    break;
		}
		
		if (ob_data.od_emc_handle != NULL) {
		    mtp_send_packet2(ob_data.od_emc_handle,
				     MA_Opcode, opcode,
				     MA_Role, MTP_ROLE_RMC,
				     MA_ObstacleVal, &on->on_expanded,
				     MA_TAG_DONE);
		}
	    }
	}
	
	on = on_succ;
    }
    
    ob_data_invariant();
}
