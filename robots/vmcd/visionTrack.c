
#include "config.h"

#include <math.h>
#include <float.h>
#include <assert.h>

#include "log.h"

#include "visionTrack.h"

extern int debug;

#define min(x, y) ((x) < (y)) ? (x) : (y)

static float curvy(float x)
{
    return (1.0f / (x * 2.0f - 2.8f)) + 1.35f;
}

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
    assert(distance_out != NULL);

    *distance_out = FLT_MAX;

    if (curr == NULL)
	return retval;
    
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

static struct vision_track *vtNextCamera(struct vision_track *vt)
{
    struct vision_track *retval = NULL;
    struct vmc_client *vc;

    vc = vt->vt_client;
    while (vt->vt_link.ln_Succ != NULL && vt->vt_client == vc) {
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }

    if (vt->vt_link.ln_Succ != NULL) {
	retval = vt;
    }

    return retval;
}

#ifdef RECORD_FRAMES
static FILE *xdat_file, *ydat_file, *tdat_file;
static unsigned int frame_count;
#endif

int vtUpdate(struct lnMinList *now,
	     struct vmc_client *vc,
	     struct mtp_packet *mp,
	     struct lnMinList *pool)
{
    int retval = 0;
    
    assert(now != NULL);
    assert(mp != NULL);

#ifdef RECORD_FRAMES
    if (xdat_file == NULL) {
	xdat_file = fopen("xframes.dat", "w");
	ydat_file = fopen("yframes.dat", "w");
	tdat_file = fopen("tframes.dat", "w");
    }
#endif

    switch (mp->data.opcode) {
    case MTP_CONTROL_ERROR:
#if 0
	fprintf(dat_file,
		"# vc %d -- empty\n",
		vc->vc_port);
#endif
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

#if 1
	    /* Adjust the cameras viewable area based on this track. */
	    if (mup->position.x < vc->vc_left)
		vc->vc_left = mup->position.x;
	    if (mup->position.x > vc->vc_right)
		vc->vc_right = mup->position.x;
	    if (mup->position.y < vc->vc_top)
		vc->vc_top = mup->position.y;
	    if (mup->position.y > vc->vc_bottom)
		vc->vc_bottom = mup->position.y;
#endif

#if 0
	    fprintf(dat_file,
		    "# vc %d -- %.2f %.2f\n",
		    vc->vc_port,
		    mup->position.x,
		    mup->position.y);
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

void vtAgeTracks(struct lnMinList *now,
		 struct lnMinList *old,
		 struct lnMinList *pool)
{
    struct vision_track *vt;
    
    assert(now != NULL);
    assert(old != NULL);

    while ((vt = (struct vision_track *)lnRemHead(old)) != NULL) {
	if (vt->vt_age < 5) {
	    float distance;
	    
	    if ((vtFindMin(vt,
			   (struct vision_track *)now->lh_Head,
			   &distance) == NULL) ||
		(distance > 0.02)) {
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

    // fprintf(dat_file, "# coalesce -- %d\n", lnEmptyList(now));

#ifdef RECORD_FRAMES
    frame_count += 1;
#endif
    
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
	    struct vision_track *vt_extra;
	    float distance;

	    while ((in_camera_count > 1) &&
		   ((vt_extra = vtFindMin(vt,
					  (struct vision_track *)
					  vtNextCamera(vt),
					  &distance)) != NULL) &&
		   (distance < 0.35)) {
		float curcam_dx, nextcam_dx, overlap, curweight, nextweight;
		struct robot_position rp;
		
		if (debug > 1) {
		    info("coalesce %.2f %.2f %.2f %d -- %.2f %.2f %.2f %d\n",
			 vt->vt_position.x, vt->vt_position.y,
			 vt->vt_position.theta, vt->vt_client->vc_port,
			 vt_extra->vt_position.x, vt_extra->vt_position.y,
			 vt->vt_position.theta, vt_extra->vt_client->vc_port);
		}

		//midpoint = (vt->vt_client->vc_right + 
		//    vt_extra->vt_client->vc_left) / 2.0f;

		//curcam_dx = midpoint - vt->vt_position.x;
		//nextcam_dx = vt_extra->vt_position.x - midpoint;

		overlap = vt->vt_client->vc_right -
		    vt_extra->vt_client->vc_left;
		curcam_dx = vt->vt_client->vc_right - vt->vt_position.x;
		nextcam_dx = vt_extra->vt_position.x - 
		    vt_extra->vt_client->vc_left;

		curweight = curvy(1.0 - curcam_dx / overlap);
		nextweight = curvy(1.0 - nextcam_dx / overlap);

#if 0
		info("  cwt %.2f / %.2f -> %.2f ~ %.2f  --  %d\n",
		     curcam_dx, overlap, curcam_dx / overlap, curweight,
		     vt->vt_client->vc_port);
		info("  nwt %.2f / %.2f -> %.2f ~ %.2f  --  %d\n",
		     nextcam_dx, overlap, nextcam_dx / overlap, nextweight,
		     vt_extra->vt_client->vc_port);
#endif
		
		rp = vt->vt_position;
		rp.x = ((curweight * vt->vt_position.x) +
			(nextweight * vt_extra->vt_position.x)) /
		    (curweight + nextweight);
		rp.y = ((curweight * vt->vt_position.y) +
			(nextweight * vt_extra->vt_position.y)) /
		    (curweight + nextweight);
		rp.theta = ((curweight * vt->vt_position.theta) +
			    (nextweight * vt_extra->vt_position.theta)) /
		    (curweight + nextweight);

#if 0
		info("  wa %.2f %.2f -> %.2f  --  %.2f %.2f -> %.2f\n",
		     vt->vt_position.x,
		     vt_extra->vt_position.x,
		     rp.x,
		     vt->vt_position.y,
		     vt_extra->vt_position.y,
		     rp.y);
#endif

		
#ifdef RECORD_FRAMES
		fprintf(xdat_file,
			"%d %.2f %.2f # coalesce  --  %.2f %.2f -- %d %d\n",
			frame_count,
			rp.x,
			(nextcam_dx > curcam_dx) ?
			vt_extra->vt_position.x : vt->vt_position.x,
			vt->vt_position.x,
			vt_extra->vt_position.x,
			vt->vt_client->vc_port,
			vt_extra->vt_client->vc_port);
		
		fprintf(ydat_file,
			"%d %.2f %.2f # coalesce  --  %.2f %.2f -- %d %d\n",
			frame_count,
			rp.y,
			(nextcam_dx > curcam_dx) ?
			vt_extra->vt_position.y : vt->vt_position.y,
			vt->vt_position.y,
			vt_extra->vt_position.y,
			vt->vt_client->vc_port,
			vt_extra->vt_client->vc_port);
		
		fprintf(tdat_file,
			"%d %.2f %.2f # coalesce  --  %.2f %.2f -- %d %d\n",
			frame_count,
			rp.theta,
			(nextcam_dx > curcam_dx) ?
			vt_extra->vt_position.theta : vt->vt_position.theta,
			vt->vt_position.theta,
			vt_extra->vt_position.theta,
			vt->vt_client->vc_port,
			vt_extra->vt_client->vc_port);
#endif
		
		vt->vt_position = rp;
		vt->vt_age = min(vt->vt_age, vt_extra->vt_age);
		
		lnRemove(&vt_extra->vt_link);
		lnAddHead(extra, &vt_extra->vt_link);
		
		in_camera_count -= 1;
	    }

	    if (in_camera_count > 1) {
#ifdef RECORD_FRAMES
		fprintf(xdat_file,
			"%d %.2f %.2f # 2 cam %d\n",
			frame_count,
			vt->vt_position.x,
			vt->vt_position.x,
			vt->vt_client->vc_port);
		fprintf(ydat_file,
			"%d %.2f %.2f # 2 cam %d\n",
			frame_count,
			vt->vt_position.y,
			vt->vt_position.y,
			vt->vt_client->vc_port);
		fprintf(tdat_file,
			"%d %.2f %.2f # 2 cam %d\n",
			frame_count,
			vt->vt_position.theta,
			vt->vt_position.theta,
			vt->vt_client->vc_port);
#endif
	    }
	}
	else {
#ifdef RECORD_FRAMES
	    fprintf(xdat_file,
		    "%d %.2f %.2f # %d\n",
		    frame_count,
		    vt->vt_position.x,
		    vt->vt_position.x,
		    vt->vt_client->vc_port);
	    fprintf(ydat_file,
		    "%d %.2f %.2f # %d\n",
		    frame_count,
		    vt->vt_position.y,
		    vt->vt_position.y,
		    vt->vt_client->vc_port);
	    fprintf(tdat_file,
		    "%d %.2f %.2f # %d\n",
		    frame_count,
		    vt->vt_position.theta,
		    vt->vt_position.theta,
		    vt->vt_client->vc_port);
#endif
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
		  (distance < 0.05)) ||
		 ((vt->vt_client != vt_start->vt_client) &&
		  (distance < 0.22))) {
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
