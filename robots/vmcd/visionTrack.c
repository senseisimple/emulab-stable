/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file visionTrack.c
 *
 */

#include "config.h"

#include <stdlib.h>

#include <math.h>
#include <float.h>
#include <assert.h>

#include "log.h"

#include "robotObject.h"
#include "visionTrack.h"

extern int debug;

/**
 * Some function I made up that provides a nice curve for computing the weight
 * for a track.  Plug the formula into gnuplot if you want to see what it looks
 * like.
 */
static float curvy(float x)
{
    return (1.0f - tanh(x * M_PI));
}

/**
 * Find the track in a list that is the minimum distance from a given track.
 * This is primarily used for matching tracks between frames, since we want to
 * always prefer the closest one.
 *
 * @param vt The track to compare against the other tracks in the list.
 * @param curr The track in a list where the search should start.  The function
 * will continue the search until it reaches the end of the list.  If NULL, the
 * function just returns NULL.
 * @param distance_inout Reference to a float that on input contains the
 * maximum allowed distance and on ouput contains the minimum distance found.
 * @return The closest track found to the given track.
 */
static struct vision_track *vtFindMin(struct vision_track *vt,
				      struct vision_track *curr,
				      float *distance_inout)
{
    struct vision_track *retval = NULL;
    int min_age = MAX_TRACK_AGE;
    float max_distance;
    
    assert(vt != NULL);
    assert(distance_inout != NULL);

    max_distance = *distance_inout;
    
    if (curr == NULL)
	return retval;
    
    while (curr->vt_link.ln_Succ != NULL) {
	float distance;

	distance = hypotf(vt->vt_position.x - curr->vt_position.x,
			  vt->vt_position.y - curr->vt_position.y);
	if (distance < max_distance) {
	    if ((curr->vt_age < min_age) ||
		((curr->vt_age <= min_age) && (distance < *distance_inout))) {
		retval = curr;
		*distance_inout = distance;
		min_age = curr->vt_age;
	    }
	}
	
	curr = (struct vision_track *)curr->vt_link.ln_Succ;
    }

    assert((retval == NULL) || (*distance_inout != max_distance));
    
    return retval;
}

/**
 * Extract tracks from a frame that are near the given track.  Used when
 * finding tracks from different cameras that need to be merged.
 *
 * @param list_out The list of tracks found to be within the distance
 * tolerance.
 * @param vt The track to compare other tracks against.
 * @param curr The track in a list where the search should start.  The function
 * will continue the search until it reaches the end of the list.
 * @param tolerance The maximum distance that other tracks have to be within
 * before they are considered "near."
 * @return The number of tracks in "list_out."
 */
static int vtExtractNear(struct lnMinList *list_out,
			 struct vision_track *vt,
			 struct vision_track *curr,
			 float tolerance)
{
    int retval = 0;
    
    assert(list_out != NULL);
    assert(vt != NULL);
    assert(tolerance > 0.0);

    if (curr == NULL)
	return retval;

    while (curr->vt_link.ln_Succ != NULL) {
	struct vision_track *vt_next;
	float distance;
	
	vt_next = (struct vision_track *)curr->vt_link.ln_Succ;

	distance = hypotf(vt->vt_position.x - curr->vt_position.x,
			  vt->vt_position.y - curr->vt_position.y);
	if (distance <= tolerance) {
	    lnRemove(&curr->vt_link);
	    lnAddTail(list_out, &curr->vt_link);
	    retval += 1;
	}
	
	curr = vt_next;
    }

    assert(lnEmptyList(list_out) || (retval > 0));

    return retval;
}

/**
 * Scan a frame for the next set of tracks that are in a different camera.
 * Often used to avoid merging tracks from the same camera, even though they
 * may be really close.
 *
 * @param vt The track in a list where the search should start.
 * @return The first vision track in the remainder of the list that was
 * detected by a different camera than the given track or NULL if there are no
 * more tracks in different cameras.
 */
static struct vision_track *vtNextCamera(struct vision_track *vt)
{
    struct vision_track *retval = NULL;
    struct vmc_client *vc;

    assert(vt != NULL);

    vc = vt->vt_client;
    while ((vt->vt_link.ln_Succ != NULL) && (vt->vt_client == vc)) {
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }

    if (vt->vt_link.ln_Succ != NULL)
	retval = vt;

    return retval;
}

int vtUpdate(struct lnMinList *now,
	     struct vmc_client *vc,
	     struct mtp_packet *mp,
	     struct lnMinList *pool)
{
    int retval = 0;
    
    assert(now != NULL);
    assert(vc != NULL);
    assert(mp != NULL);
    assert(pool != NULL);

    if (debug > 2) {
	mtp_print_packet(stdout, mp);
	fflush(stdout);
    }
    
    switch (mp->data.opcode) {
    case MTP_CONTROL_ERROR:
	retval = 1;
	break;
    case MTP_UPDATE_POSITION:
	{
	    struct mtp_update_position *mup;
	    struct vision_track *vt;
	    
	    mup = &mp->data.mtp_payload_u.update_position;
	    vt = (struct vision_track *)lnRemHead(pool);
	    assert(vt != NULL);

	    vt->vt_position = mup->position;
	    vt->vt_client = vc;
	    vt->vt_age = 0;
	    vt->vt_userdata = NULL;


	    /* zero out the smoothing data for this 
	     * track!
	     */
	    //vt->ma.oldest_index = 0;
	    //vt->ma.number_valid_positions = 0;


	    lnAddTail(now, &vt->vt_link);

	    /* Adjust the cameras viewable area based on this track. */
	    if (mup->position.x < vc->vc_left)
		vc->vc_left = mup->position.x;
	    if (mup->position.x > vc->vc_right)
		vc->vc_right = mup->position.x;
	    if (mup->position.y < vc->vc_top)
		vc->vc_top = mup->position.y;
	    if (mup->position.y > vc->vc_bottom)
		vc->vc_bottom = mup->position.y;

	    retval = (mup->status == MTP_POSITION_STATUS_CYCLE_COMPLETE);
	}
	break;
    default:
	error("unhandled vmc-client packet %d\n", mp->data.opcode);
	break;
    }
    
    return retval;
}

void vtAgeTracks(struct lnMinList *now,
		 struct lnMinList *old,
		 struct lnMinList *pool)
{
    struct vision_track *vt;
    
    assert(now != NULL);
    assert(old != NULL);

    while ((vt = (struct vision_track *)lnRemHead(old)) != NULL) {
	if (vt->vt_age < MAX_TRACK_AGE) {
	    float distance = MERGE_TOLERANCE;
	    
	    if (vtFindMin(vt,
			  (struct vision_track *)now->lh_Head,
			  &distance) == NULL) {
		vt->vt_age += 1;
		lnAddHead(now, &vt->vt_link);
		vt = NULL;
	    }
	}

	if (vt != NULL)
	    lnAddHead(pool, &vt->vt_link);
    }
}

void vtCopyTracks(struct lnMinList *dst,
		  struct lnMinList *src,
		  struct lnMinList *pool)
{
    struct vision_track *vt;
    
    assert(dst != NULL);
    assert(src != NULL);
    assert(pool != NULL);

    vt = (struct vision_track *)src->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	struct vision_track *vt_copy = (struct vision_track *)lnRemHead(pool);

	*vt_copy = *vt;
	lnAddTail(dst, &vt_copy->vt_link);
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
}

void vtCoalesce(struct lnMinList *extra,
		struct lnMinList *now,
		struct vmc_client *vc,
		unsigned int vc_len)
{
    struct vision_track *vt;

    assert(extra != NULL);
    assert(now != NULL);
    assert(vc != NULL);
    assert(vc_len > 0);

    /*
     * Walk the list and munge it along the way.  So, as we progress through
     * the list, any tracks before the current one will be completely coalesced
     * and any tracks after will be those remaining after previous coalesces.
     */
    vt = (struct vision_track *)now->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	int in_camera_count = 0, lpc;

	/*
	 * Figure out how many cameras this track might be viewable in so we
	 * can coalesce them.  XXX Should we pad the camera bounds by
	 * COALESCE_TOLERANCE since the cameras might be off by a bit?
	 */
	for (lpc = 0; lpc < vc_len; lpc++) {
	    if ((vt->vt_position.x >= vc[lpc].vc_left) &&
		(vt->vt_position.x <= vc[lpc].vc_right) &&
		(vt->vt_position.y >= vc[lpc].vc_top) &&
		(vt->vt_position.y <= vc[lpc].vc_bottom)) {
		in_camera_count += 1;
	    }
	}

	assert(in_camera_count > 0);

	/* 
	 * we want to give a slow move-over to the new camera -- i.e., we 
	 * really don't want to see the position bouncing back and forth
	 * between cameras...
	 * 
	 * how to do this ... ?
	 * 
	 * forget slow for now, let's just try to pick the best one.
	 *   - WENT WITH THIS APPROACH FOR NOW!!!
	 * 
	 * the problem would be if a robot sits right on the fricking 
	 * boundary; if it does, then we could easily get positions that
	 * switch off between cameras every ...
	 *
	 * but wait! we can keep track of orientation for each robot's track;
	 * since we have that, we can choose which camera is the "overlap"
	 * camera, and enforce some small buffering to ensure that the robot
	 * is really moving into the next camera before we allow the switch to
	 * proceed.
	 * 
	 */

	if (in_camera_count > 1) {
	    struct lnMinList near;
	    int rc;
	    
	    lnNewList(&near);
	    /*
	     * Extract any other tracks that are close to the current one and
	     * in the remaining cameras.
	     */
	    if ((rc = vtExtractNear(&near,
				    vt,
				    vtNextCamera(vt),
				    COALESCE_TOLERANCE)) > 0) {
		float weight, total_weight = 0.0;
		struct vision_track *vt_near;
		struct robot_position rp;
		
		if (debug > 2) {
		    printf("start %.2f %.2f\n", vt->vt_position.x,
			   vt->vt_position.y);
		}

		/* Compute the weight for this track, younger is better. */
		weight = curvy((float)vt->vt_age /
			       (float)MAX_TRACK_AGE);
		total_weight += weight;

		/* Initialize the position data with the current track. */
		rp.x = vt->vt_position.x * weight;
		rp.y = vt->vt_position.y * weight;
		rp.theta = vt->vt_position.theta;
		rp.timestamp = vt->vt_position.timestamp;

		/* Add in the position data from the other tracks. */
		vt_near = (struct vision_track *)near.lh_Head;
		while (vt_near->vt_link.ln_Succ != NULL) {
		    if (debug > 2) {
			printf("merge %.2f %.2f\n", vt_near->vt_position.x,
			       vt_near->vt_position.y);
		    }
		    
		    weight = curvy((float)vt_near->vt_age /
				   (float)MAX_TRACK_AGE);
		    total_weight += weight;
		    rp.x += vt_near->vt_position.x * weight;
		    rp.y += vt_near->vt_position.y * weight;
		    // rp.theta += vt_near->vt_position.theta;
		    vt->vt_age = min(vt->vt_age, vt_near->vt_age);
		    vt->vt_position.timestamp =
			max(vt->vt_position.timestamp,
			    vt_near->vt_position.timestamp);
		    
		    vt_near = (struct vision_track *)vt_near->vt_link.ln_Succ;
		}

		/* Return merged tracks to the pool. */
		lnAppendList(extra, &near);

		rp.x /= total_weight;
		rp.y /= total_weight;
		if (debug > 2) {
		    printf("final %.2f %.2f\n", rp.x, rp.y);
		}
		// rp.theta /= rc;
		vt->vt_position = rp;
	    }
	}
	
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
}

void vtMatch(struct lnMinList *pool,
	     struct lnMinList *prev,
	     struct lnMinList *now)
{
    struct vision_track *vt;

    assert(pool != NULL);
    assert(prev != NULL);
    assert(now != NULL);

    /*
     * Walk through the tracks in the current frame trying to find tracks in
     * the previous frame, within some threshold.
     */
    vt = (struct vision_track *)now->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	float distance = MERGE_TOLERANCE, dummy = MERGE_TOLERANCE;
	struct vision_track *vt_prev;
	void dump_vision_list(struct lnMinList *list);

	/*
	 * Check both ways, the current track is the close to one in the
	 * previous frame, and the track from the previous frame is closest to
	 * the one in the current frame.
	 */
	if (((vt_prev = vtFindMin(vt,
				  (struct vision_track *)prev->lh_Head,
				  &distance)) != NULL) &&
	    (vtFindMin(vt_prev,
		       (struct vision_track *)now->lh_Head,
		       &dummy) == vt)) {
		vt->vt_userdata = vt_prev->vt_userdata;
		
		vt->ma = vt_prev->ma;
		
		lnRemove(&vt_prev->vt_link);
		lnAddHead(pool, &vt_prev->vt_link);
	}
	else {
#if 0
	    info("no match %.2f %.2f\n", vt->vt_position.x, vt->vt_position.y);
	    dump_vision_list(now);
	    info("==\n");
	    dump_vision_list(prev);
#endif
	}
	
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
}

void vtUnknownTracks(struct lnMinList *unknown, struct lnMinList *mixed)
{
    struct vision_track *vt;

    assert(unknown != NULL);
    assert(mixed != NULL);
    
    vt = (struct vision_track *)mixed->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	struct vision_track *vt_succ;
	
	vt_succ = (struct vision_track *)vt->vt_link.ln_Succ;
	
	if (vt->vt_userdata == NULL) {
	    lnRemove(&vt->vt_link);
	    lnAddTail(unknown, &vt->vt_link);
	}
	
	vt = vt_succ;
    }
}

struct vision_track *vtFindWiggle(struct lnMinList *start,
				  struct lnMinList *now)
{
    struct vision_track *vt, *retval = NULL;

    assert(start != NULL);
    assert(now != NULL);

    /*
     * Walk through the current frame searching for tracks that have a small
     * displacement from the start frame and large orientation change.
     */
    vt = (struct vision_track *)now->lh_Head;
    while ((vt->vt_link.ln_Succ != NULL) && (retval == NULL)) {
	struct vision_track *vt_start;
	float distance = FLT_MAX;

	info("check %f %f %f\n",
	     vt->vt_position.x,
	     vt->vt_position.y,
	     vt->vt_position.theta);
	if ((vt_start = vtFindMin(vt,
				  (struct vision_track *)start->lh_Head,
				  &distance)) == NULL) {
	    /* No match. */
	}
	else if (((vt->vt_client == vt_start->vt_client) &&
		  (distance < WIGGLE_TOLERANCE)) ||
		 ((vt->vt_client != vt_start->vt_client) &&
		  (distance < WIGGLE_DIFFCAM_TOLERANCE))) {
	    float diff;

	    diff = mtp_theta(mtp_theta(vt_start->vt_position.theta) -
			     mtp_theta(vt->vt_position.theta));
	    info("hit for %.2f %.2f %.2f -- %.2f %.2f %.2f = %.2f\n"
		 "  %f -- %f\n",
		 vt->vt_position.x,
		 vt->vt_position.y,
		 vt->vt_position.theta,
		 vt_start->vt_position.x,
		 vt_start->vt_position.y,
		 vt_start->vt_position.theta,
		 diff,
		 vt->vt_position.timestamp,
		 vt_start->vt_position.timestamp);
	    if (diff > (M_PI - M_PI_4) || (diff < (-M_PI + M_PI_4))) {
		retval = vt;
		info(" found it %p\n", retval);
	    }
	}
	info("dist %p %f\n", vt_start, distance);
	
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
    
    return retval;
}

static float orientationToCircle(float o) {
    if (o < 0.0f && o >= -M_PI) {
	return (o + 2*M_PI);
    }
    else {
	return o;
    }
}

static float orientationToSemiCircle(float o) {
    if (o > M_PI && o < 2*M_PI) {
	return (o - 2*M_PI);
    }
    else {
	return o;
    }
}

/** 
 * Given a moving average, updates it with the data in new and outputs the 
 * calculated average in avg.
 * 
 * @param ma The moving average metadata.
 * @param new The data to add to the moving average.
 * @param avg The output from this function; contains the latest moving avg.
 */
static void updateMovingAverage(struct moving_average *ma,
				struct robot_position *new) {
    int i;
    int vp;
    int quads[4] = {0,0,0,0};
    int quad_count = 0;
    
    assert(ma != NULL);
    assert(ma->positions != NULL);
    assert(new != NULL);

    /* setup */
    
    /* update data samples array */
    ma->positions[ma->oldest_index] = *new;
    
    /* point to the next piece of data to be replaced */
    ma->oldest_index = (ma->oldest_index + 1) % ma->positions_len;
    /* if the ring buffer isn't full yet, incr the number of valid data pts */
    if (ma->number_valid_positions < ma->positions_len) {
	++(ma->number_valid_positions);
    }
    
    ma->current_avg.x = 0.0f;
    ma->current_avg.y = 0.0f;
    ma->current_avg.theta = 0.0f;

    vp = ma->number_valid_positions;

    /* make sure that the theta is BETWEEN 0 and 2PI -- otherwise an improper
     * average could be calculated (i.e., if it's flipping between +PI and -PI)
     */

    /** 
     * this gets tricky.  here's the algorithm, and it should do well in 
     * almost every case (certainly, in every case for which incoming data
     * is "sensible."
     *
     * 1) see how many and which quadrants we have data for.
     * 2) if |q| > 2, return latest orientation as avg.  Can't do much better
     *    here, except maybe to cut out all data that's not in the two most
     *    populous quadrants.
     * 3) if |q| == 1, calculate the simple average.
     * 4) if |q| == 2:
     *      if q == {2,3}, convert to full circle coords (i.e., convert 
     *         [-PI,0) -> [PI,2PI), and calculate
     *         simple average.  If answer is in [-PI,2PI], convert back to
     *         [-PI,0).
     *      if qi are NOT adjacent, return latest orientation as average.
     *      if qi are adjacent, calculate simple average.
     */
    
    for (i = 0; i < vp; ++i) {
	float tt = ma->positions[i].theta;

	if (tt >= 0.0f && tt < M_PI_2) {
	    if (quads[0] == 0) {
		++quad_count;
	    }
	    ++(quads[0]);
	}
	else if (tt >= M_PI_2 && tt <= M_PI) {
	    if (quads[1] == 0) {
                ++quad_count;
            }
	    ++(quads[1]);
	}
	else if (tt >= -M_PI && tt <= -M_PI_2) {
	    if (quads[2] == 0) {
                ++quad_count;
            }
	    ++(quads[2]);
	}
	else if (tt > -M_PI_2 && tt < 0.0f) {
	    if (quads[3] == 0) {
                ++quad_count;
            }
	    ++(quads[3]);
	}
	else {
	    info("FATAL ERROR: orientation not in range 0,PI or 0,-PI !!!\n");
	    assert(0);
	}
    }

    /* handle positions like normal */
    for (i = 0; i < vp; ++i) {
	ma->current_avg.x += ma->positions[i].x;
	ma->current_avg.y += ma->positions[i].y;
    }
    
    ma->current_avg.x = ma->current_avg.x / vp;
    ma->current_avg.y = ma->current_avg.y / vp;
    
    /* now, handle the various orientations we may have... */
    if (quad_count > 2) {
	ma->current_avg.theta = new->theta;
	info("S WARNING: had to use only latest theta "
	     "(> 2 quadrants); no avg!\n");
	info("S QUADS: q0 = %d, q1 = %d, q2 = %d, q3 = %d\n",
	     quads[0],quads[1],quads[2],quads[3]);
    }
    else if (quad_count == 1) {
	/* calculate simple average, a la positions */
	for (i = 0; i < vp; ++i) {
	    ma->current_avg.theta += ma->positions[i].theta;
	}
	ma->current_avg.theta = ma->current_avg.theta / vp;
    }
    else if (quad_count == 2) {
	if (quads[1] && quads[2]) {
	    float *to = (float *)malloc(sizeof(float)*(vp));

	    /* convert, then calculate avg... then convert avg back */
	    for (i = 0; i < vp; ++i) {
		to[i] = orientationToCircle(ma->positions[i].theta);
		ma->current_avg.theta += to[i];
	    }
	    
	    ma->current_avg.theta = 
		orientationToSemiCircle(ma->current_avg.theta / vp);

	    free(to);

	}
	else if ((quads[0] && quads[1]) || 
		 (quads[2] && quads[3]) ||
		 (quads[0] && quads[3])) {
	     /* calculate simple avg */
	     for (i = 0; i < vp; ++i) {
		 ma->current_avg.theta += ma->positions[i].theta;
	     }
	     
	     ma->current_avg.theta = ma->current_avg.theta / vp;
	}
	else {
	    /* set current to new orient */
	    ma->current_avg.theta = new->theta;
	    info("S WARNING: had to use only latest theta "
		 "(non-adj); no avg!\n");
	}
	
    }

#if 1
    if (debug > 2) {
	info("UPDATING moving avg: %d valid positions\n",vp);
    }

#endif

    return;
}

void vtSmooth(struct lnMinList *tracks) {
    struct vision_track *vt = NULL;
    struct robot_object *ro = NULL;
    struct moving_average *ma = NULL;
    float distance_delta = 0.0f;
    float theta_delta = 0.0f;
    
    assert(tracks != NULL);
    
    vt = (struct vision_track *)tracks->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	if ((ro = vt->vt_userdata) != NULL) {
	    ma = &(vt->ma);
	    
	    /* using the latest position in ro->position, calc moving avg */
	    updateMovingAverage(ma,
				&(vt->vt_position));

	    if (debug > 2) {
		info("SMOOTHED position for robot %d from (%f,%f,%f) to "
		     "(%f,%f,%f)\n",
		     ro->ro_id,
		     vt->vt_position.x,
		     vt->vt_position.y,
		     vt->vt_position.theta,
		     ma->current_avg.x,
		     ma->current_avg.y,
		     ma->current_avg.theta
		     );
	    }

	    /* figure out how much the posit will change from the last
	     * one reported... this is important so we know if we mess up
	     * wiggles
	     */
	    distance_delta = hypotf(vt->vt_position.x - ma->current_avg.x,
				    vt->vt_position.y - ma->current_avg.y);
	    theta_delta = (float)abs(vt->vt_position.theta - 
				     ma->current_avg.theta);
	    if (debug > 2) {
		info("  DELTAS were %f dist and %f angle.\n",
		     distance_delta,
		     theta_delta);
	    }

	    /*  update the position... this is a bit sneaky... but this way
	     * we don't have to change oodles of code.
	     */
	    vt->vt_position.x = ma->current_avg.x;
	    vt->vt_position.y = ma->current_avg.y;
	    vt->vt_position.theta = ma->current_avg.theta;
	    // don't update the timestamp!
	    
	}
	
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
    
    return;
}
