/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file visionTrack.c
 *
 */

#include "config.h"

#include <math.h>
#include <float.h>
#include <assert.h>

#include "log.h"

#include "visionTrack.h"

extern int debug;

static float curvy(float x)
{
    return (1.0f / (x * 2.0f - 2.8f)) + 1.35f;
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
 * @param distance_out Reference to a float where the minimum distance found
 * should be stored.
 * @return The closest track found to the given track.
 */
static struct vision_track *vtFindMin(struct vision_track *vt,
				      struct vision_track *curr,
				      float *distance_out)
{
    struct vision_track *retval = NULL;

    assert(vt != NULL);
    assert(distance_out != NULL);

    *distance_out = FLT_MAX;

    if (curr == NULL)
	return retval;
    
    while (curr->vt_link.ln_Succ != NULL) {
	float distance;

	distance = hypotf(vt->vt_position.x - curr->vt_position.x,
			  vt->vt_position.y - curr->vt_position.y);
	if (distance < *distance_out) {
	    retval = curr;
	    *distance_out = distance;
	}
	
	curr = (struct vision_track *)curr->vt_link.ln_Succ;
    }

    assert((retval == NULL) || (*distance_out != FLT_MAX));
    
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
	    float distance;
	    
	    if ((vtFindMin(vt,
			   (struct vision_track *)now->lh_Head,
			   &distance) == NULL) ||
		(distance > MERGE_TOLERANCE)) {
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

    vt = (struct vision_track *)now->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	int in_camera_count = 0, lpc;

	/*
	 * Figure out how many cameras this track might be viewable in so we
	 * can coalesce them.
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
	    if ((rc = vtExtractNear(&near,
				    vt,
				    vtNextCamera(vt),
				    COALESCE_TOLERANCE)) > 0) {
		struct robot_position rp = vt->vt_position;
		struct vision_track *vt_near;
		
		if (debug > 2) {
		    printf("start %.2f %.2f\n", vt->vt_position.x,
			   vt->vt_position.y);
		}
		vt_near = (struct vision_track *)near.lh_Head;
		while (vt_near->vt_link.ln_Succ != NULL) {
		    if (debug > 2) {
			printf("merge %.2f %.2f\n", vt_near->vt_position.x,
			       vt_near->vt_position.y);
		    }
		    rp.x += vt_near->vt_position.x;
		    rp.y += vt_near->vt_position.y;
		    // rp.theta += vt_near->vt_position.theta;
		    vt->vt_age = min(vt->vt_age, vt_near->vt_age);
		    
		    vt_near = (struct vision_track *)vt_near->vt_link.ln_Succ;
		}
		lnAppendList(extra, &near);

		rp.x /= rc + 1;
		rp.y /= rc + 1;
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
	struct vision_track *vt_prev;
	float distance;
	void dump_vision_list(struct lnMinList *list);
	
	if ((vt_prev = vtFindMin(vt,
				 (struct vision_track *)prev->lh_Head,
				 &distance)) != NULL) {
	    if (distance <= 0.06) {
		vt->vt_userdata = vt_prev->vt_userdata;
		lnRemove(&vt_prev->vt_link);
		lnAddHead(pool, &vt_prev->vt_link);
	    }
	    else {
#if 0
		info("too far %.2f\n", distance);
#endif
	    }
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
	float distance;

	info("check %f %f %f\n",
	     vt->vt_position.x,
	     vt->vt_position.y,
	     vt->vt_position.theta);
	if ((vt_start = vtFindMin(vt,
				  (struct vision_track *)start->lh_Head,
				  &distance)) == NULL) {
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
