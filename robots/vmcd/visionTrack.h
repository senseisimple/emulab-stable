/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _vision_track_h
#define _vision_track_h

/**
 * @file visionTrack.h
 */

#include "mtp.h"
#include "listNode.h"

#include "vmcd.h"

/**
 * The maximum distance in meters for which tracks from different cameras will
 * be considered the same and coalesced.
 */
#define COALESCE_TOLERANCE 0.075

/**
 * The maximum distance in meters for which a track is considered the same from
 * one frame to the next.
 */
#define MERGE_TOLERANCE 0.075

/**
 * The maximum number of frames that a track can miss before it is considered
 * lost.
 */
#define MAX_TRACK_AGE 5

/**
 * Structure used to keep a moving average of last N positions.
 */
struct moving_average {
    struct robot_position positions[15];   /*< ring buffer of most recent posits */
    int positions_len;                 /*< length of buffer */
    int number_valid_positions;        /*< the buffer may not be full... */
    
    /* don't want to shift the contents of the buffer every time, doh */
    int oldest_index;                  /*< this is the one we should replace */
    struct robot_position current_avg; /*< the last calc'd avg */
};

/**
 * The maximum distance, in meters, that will be tolerated between an initial
 * wiggle frame and the final.
 */
#define WIGGLE_TOLERANCE 0.05

/**
 * The maximum distance, in meters, that will be tolerated between an initial
 * wiggle frame in one camera and the final frame in a different camera.
 */
#define WIGGLE_DIFFCAM_TOLERANCE 0.10

/**
 * Structure used to manage fiducials detected by the vision system.
 */
struct vision_track {
    struct lnMinNode vt_link;		/*< Linked list header. */
    struct robot_position vt_position;	/*< Fiducial position/orientation. */
    struct vmc_client *vt_client;	/*< Camera that detected the track. */
    int vt_age;				/*< Age of the track (lower=younger) */
    void *vt_userdata;			/*< Data attached to the track. */
    struct moving_average ma;           /*< smooth out the positions... */
};

/**
 * Update the list of tracks for the current frame with the tracks from a given
 * vmc-client.  The dimensions of the camera will also be adjusted to ensure
 * the track's position falls within the bounds.
 *
 * @param now The track list for the current frame, any new tracks will be
 * added to this.
 * @param client The reference to the vmc-client that produced a track.
 * @param mp The packet received from the client.
 * @param pool The pool of nodes to draw vision_track objects from.
 * @return True if all of the tracks have been received from this client for
 * the current frame.
 */
int vtUpdate(struct lnMinList *now,
	     struct vmc_client *client,
	     struct mtp_packet *mp,
	     struct lnMinList *pool);

/**
 * Move tracks from previous frames into the current frame if they are still
 * young and missing from the current frame.  Tracks that are too old or match
 * a track in the current frame are returned to the pool.
 *
 * @param now The list of tracks in the current frame.
 * @param old The list of tracks from the previous frame.
 * @param pool The list to return dead tracks to.
 */
void vtAgeTracks(struct lnMinList *now,
		 struct lnMinList *old,
		 struct lnMinList *pool);

/**
 * Copy tracks in the source frame to the destination frame.
 *
 * @param dst The list to add new tracks to.
 * @param src The list of tracks to duplicate.
 * @param pool The list of free nodes to draw from.
 */
void vtCopyTracks(struct lnMinList *dst,
		  struct lnMinList *src,
		  struct lnMinList *pool);

/**
 * Coalesce tracks that are in an area where cameras are overlapping.  This
 * function will produce a frame with the canonical set of tracks.
 *
 * @param extra The list to add any duplicate tracks found in the current
 * frame.
 * @param now The list of tracks from all of the cameras.  The list is expected
 * to be organized so that all of the tracks from a camera are grouped
 * together.
 * @param vc The array of cameras, coalescing is based on the camera's viewable
 * area.
 * @param vc_len The length of the vc array.
 */
void vtCoalesce(struct lnMinList *extra,
		struct lnMinList *now,
		struct vmc_client *vc,
		unsigned int vc_len);

/**
 * Match any tracks from the current frame against the previous frame.  Any
 * matches will have their vt_userdata value copied over.  Any tracks from the
 * previous frame that weren't matched, will have their vt_age value
 * incremented and be appended to the current frame if their age value is less
 * than 5.  By keeping older frames around, we can deal with tracks that skip
 * a few frames, which seems to happen at the edge of the camera's viewable
 * area.
 *
 * @param pool The pool to add any older, or matched nodes, to.
 * @param prev The previous frame to match.
 * @param now The current frame.
 */
void vtMatch(struct lnMinList *pool,
	     struct lnMinList *prev,
	     struct lnMinList *now);

/**
 * Move any tracks whose vt_userdata == NULL from one list to another.
 *
 * @param unknown The destination for any tracks that have no userdata value.
 * @param mixed The list of tracks to sort.
 */
void vtUnknownTracks(struct lnMinList *unknown, struct lnMinList *mixed);

/**
 * Find a vision_track that changed 180 degrees from a previous frame to the
 * current frame.
 *
 * @param start A snapshot of unknown tracks from before the wiggle started.
 * @param now The current frame.
 * @param return The matched vision_track or NULL if none could be found.
 */
struct vision_track *vtFindWiggle(struct lnMinList *start,
				  struct lnMinList *now);

/**
 * Taking the latest position, augment the moving average for each track.  This
 * should be done AFTER a vtMatch so that we only avg on valid data.  Doing 
 * smoothing after everything else will allow the current matching algorithms 
 * to continue to work.
 */
void vtSmoothSMA(struct lnMinList *tracks);

void vtSmoothEWMA(struct lnMinList *tracks);

#endif
