
#ifndef _vision_track_h
#define _vision_track_h

#include "mtp.h"
#include "listNode.h"

#include "vmcd.h"

struct vision_track {
    struct lnMinNode vt_link;
    struct robot_position vt_position;
    int vt_age;
    void *vt_userdata;
};

int vtUpdate(struct lnMinList *now,
	     struct vmc_client *client,
	     struct mtp_packet *mp,
	     struct lnMinList *pool);

void vtCoalesce(struct lnMinList *extra,
		struct lnMinList *now,
		struct vmc_client *vc,
		unsigned int vc_len);

void vtMatch(struct lnMinList *pool,
	     struct lnMinList *prev,
	     struct lnMinList *now);

void vtUnknownTracks(struct lnMinList *unknown, struct lnMinList *mixed);

struct vision_track *vtFindWiggle(struct lnMinList *start,
				  struct lnMinList *now);

#endif
