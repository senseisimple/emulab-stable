
#ifndef _vision_track_h
#define _vision_track_h

#include "mtp.h"
#include "listNode.h"

#include "vmcd.h"

struct vision_track {
    struct lnMinNode vt_link;
    struct robot_position vt_position;
    struct vmc_client *vt_client;
    int vt_age;
    void *vt_userdata;
};

/**
 * Update the list of tracks for the current frame with the tracks from a given
 * vmc-client.
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
 * Coalesce tracks that are in an area where cameras are overlapping.
 *
 * @param extra The list to add any duplicate tracks found in the current
 * frame.
 * @param now The list of tracks from all of the cameras.
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

#endif
