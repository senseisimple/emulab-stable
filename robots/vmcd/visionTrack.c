
#include "config.h"

#include <math.h>
#include <float.h>
#include <assert.h>

#include "log.h"

#include "visionTrack.h"

/**
 * Find the track in a list that is the minimum distance from a given track.
 *
 * @param vt The track to compare against the other tracks in the list.
 * @param curr The track in a list where the search should start.  The function
 * will continue the search until it reaches the end of the list.
 * @param distance_out Reference to a float where the minimum distance found
 * should be stored.
 */
static struct vision_track *vtFindMin(struct vision_track *vt,
				      struct vision_track *curr,
				      float *distance_out)
{
    struct vision_track *retval = NULL;

    assert(vt != NULL);
    assert(curr != NULL);
    assert(distance_out != NULL);

    *distance_out = FLT_MAX;

    while (curr->vt_link.ln_Succ != NULL) {
	float distance;

	distance = hypotf(vt->vt_position.x - curr->vt_position.x,
			  vt->vt_position.y - curr->vt_position.y);
#if 0
	printf("  min %f %f - %f %f = %f\n",
	       vt->vt_position.x,
	       vt->vt_position.y,
	       curr->vt_position.x,
	       curr->vt_position.y,
	       distance);
#endif
	if (distance < *distance_out) {
	    retval = curr;
	    *distance_out = distance;
	}
	
	curr = (struct vision_track *)curr->vt_link.ln_Succ;
    }
    
    return retval;
}

int vtUpdate(struct lnMinList *now,
	     struct vmc_client *vc,
	     struct mtp_packet *mp,
	     struct lnMinList *pool)
{
    int retval = 0;
    
    assert(now != NULL);
    assert(mp != NULL);

    switch (mp->data.opcode) {
    case MTP_CONTROL_ERROR:
	{
	    struct mtp_control *mc;

	    mc = &mp->data.mtp_payload_u.error;
	    
	    retval = (mc->code == MTP_POSITION_STATUS_CYCLE_COMPLETE);
	}
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

#if 0
	    printf("vc %p %f %f %f %f\n",
		   vc,
		   vc->vc_left,
		   vc->vc_right,
		   vc->vc_top,
		   vc->vc_bottom);
#endif
	    retval = (mup->status == MTP_POSITION_STATUS_CYCLE_COMPLETE);
	}
	break;
    default:
	error("unhandled vmc-client packet %d\n", mp->data.opcode);
	break;
    }
    
    return retval;
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
#if 0
	    printf("vc2 %f %f -- %f %f %f %f\n",
		   vt->vt_position.x,
		   vt->vt_position.y,
		   vc[lpc].vc_left,
		   vc[lpc].vc_right,
		   vc[lpc].vc_top,
		   vc[lpc].vc_bottom);
#endif
	    if ((vt->vt_position.x >= vc[lpc].vc_left) &&
		(vt->vt_position.x <= vc[lpc].vc_right) &&
		(vt->vt_position.y >= vc[lpc].vc_top) &&
		(vt->vt_position.y <= vc[lpc].vc_bottom)) {
		in_camera_count += 1;
	    }
	}

	assert(in_camera_count > 0);

	if (in_camera_count > 1) {
	    struct vision_track *vt_extra;
	    float distance;
	    
	    while ((in_camera_count > 1) &&
		   ((vt_extra = vtFindMin(vt,
					  (struct vision_track *)
					  vt->vt_link.ln_Succ,
					  &distance)) != NULL) &&
		   (distance < 0.35)) {
		printf("coalesce %f %f  --  %f %f\n",
		       vt->vt_position.x, vt->vt_position.y,
		       vt_extra->vt_position.x, vt_extra->vt_position.y);
		
		lnRemove(&vt_extra->vt_link);
		lnAddHead(extra, &vt_extra->vt_link);

		in_camera_count -= 1;
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
	
	if ((vt_prev = vtFindMin(vt,
				 (struct vision_track *)prev->lh_Head,
				 &distance)) != NULL) {
	    if (distance < 0.30) {
		vt->vt_userdata = vt_prev->vt_userdata;
		lnRemove(&vt_prev->vt_link);
		lnAddHead(pool, &vt_prev->vt_link);
	    }
	}
	
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }

    /*
     * Walk through the previous frame copying any young tracks to the current
     * frame.
     */
    vt = (struct vision_track *)prev->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	struct vision_track *vt_next;

	vt_next = (struct vision_track *)vt->vt_link.ln_Succ;
	if (vt->vt_age < 5) {
	    vt->vt_age += 1;
	    lnRemove(&vt->vt_link);
	    lnAddHead(now, &vt->vt_link);
	}

	vt = vt_next;
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

	printf("check %f %f %f\n",
	       vt->vt_position.x,
	       vt->vt_position.y,
	       vt->vt_position.theta);
	if ((vt_start = vtFindMin(vt,
				  (struct vision_track *)start->lh_Head,
				  &distance)) == NULL) {
	}
	else if (distance < 0.05) {
	    float diff;

	    diff = mtp_theta(mtp_theta(vt_start->vt_position.theta) -
			     mtp_theta(vt->vt_position.theta));
	    printf("diff = %f  %f - %f\n",
		   diff,
		   mtp_theta(vt_start->vt_position.theta),
		   mtp_theta(vt->vt_position.theta));
	    printf("diff2 = %f  %f - %f\n",
		   mtp_theta(vt_start->vt_position.theta) -
		   mtp_theta(vt->vt_position.theta),
		   vt_start->vt_position.theta,
		   vt->vt_position.theta);
	    if (diff > (M_PI - M_PI_4) || (diff < (-M_PI + M_PI_4))) {
		retval = vt;
		printf(" found it %p\n", retval);
	    }
	}
	printf("dist %p %f\n", vt_start, distance);
	
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
    
    return retval;
}
