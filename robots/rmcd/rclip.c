/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "rclip.h"

int rc_rect_intersects(rc_rectangle_t r1, rc_rectangle_t r2)
{
    int retval = 0;

    assert(r1 != NULL);
    assert(r2 != NULL);

    retval = !((r1->xmin > (r2->xmax)) ||
	       (r1->ymin > (r2->ymax)) ||
	       (r2->xmin > (r1->xmax)) ||
	       (r2->ymin > (r1->ymax)));
    
    return retval;
}

rc_code_t rc_compute_code(float x, float y, rc_rectangle_t r)
{
    rc_code_t c = 0;

    assert(r != NULL);
    
    if (y > r->ymax)
	c |= RCF_TOP;
    else if (y < r->ymin)
	c |= RCF_BOTTOM;
    if (x > r->xmax)
	c |= RCF_RIGHT;
    else if (x < r->xmin)
	c |= RCF_LEFT;
    
    return c;
}

rc_point_t rc_corner(rc_code_t rc, rc_point_t rp, rc_rectangle_t r)
{
    assert(rc != 0);
    assert(rp != NULL);
    assert(r != NULL);

    switch (rc) {
    case RCF_TOP|RCF_LEFT:
	rp->x = r->xmin;
	rp->y = r->ymin;
	break;
    case RCF_TOP|RCF_RIGHT:
	rp->x = r->xmax;
	rp->y = r->ymin;
	break;
    case RCF_BOTTOM|RCF_LEFT:
	rp->x = r->xmin;
	rp->y = r->ymax;
	break;
    case RCF_BOTTOM|RCF_RIGHT:
	rp->x = r->xmax;
	rp->y = r->ymax;
	break;

    default:
	assert(0);
	break;
    }

    return rp;
}

int rc_clip_line(rc_line_t line, rc_rectangle_t clip)
{
    rc_code_t C0, C1, C;
    float x, y;

    assert(line != NULL);
    assert(clip != NULL);
    
    C0 = rc_compute_code(line->x0, line->y0, clip);
    C1 = rc_compute_code(line->x1, line->y1, clip);
    
    for (;;) {
	/* trivial accept: both ends in rectangle */
	if ((C0 | C1) == 0) {
	    return 1;
	}
	
	/* trivial reject: both ends on the external side of the rectangle */
	if ((C0 & C1) != 0)
	    return 0;
	
	/* normal case: clip end outside rectangle */
	C = C0 ? C0 : C1;
	if (C & RCF_TOP) {
	    x = line->x0 + (line->x1 - line->x0) *
		(clip->ymax - line->y0) / (line->y1 - line->y0);
	    y = clip->ymax;
	} else if (C & RCF_BOTTOM) {
	    x = line->x0 + (line->x1 - line->x0) *
		(clip->ymin - line->y0) / (line->y1 - line->y0);
	    y = clip->ymin;
	} else if (C & RCF_RIGHT) {
	    x = clip->xmax;
	    y = line->y0 + (line->y1 - line->y0) *
		(clip->xmax - line->x0) / (line->x1 - line->x0);
	} else {
	    x = clip->xmin;
	    y = line->y0 + (line->y1 - line->y0) *
		(clip->xmin - line->x0) / (line->x1 - line->x0);
	}
	
	/* set new end point and iterate */
	if (C == C0) {
	    line->x0 = x; line->y0 = y;
	    C0 = rc_compute_code(line->x0, line->y0, clip);
	} else {
	    line->x1 = x; line->y1 = y;
	    C1 = rc_compute_code(line->x1, line->y1, clip);
	}
    }
    
    /* notreached */
    assert(0);
}
