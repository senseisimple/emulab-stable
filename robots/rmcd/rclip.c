
#include "config.h"

#include <stdio.h>
#include <assert.h>

#include "rclip.h"

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

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

rc_code_t rc_compute_closest(float x, float y, rc_rectangle_t r)
{
    float left, top, bottom, right, closest;
    rc_code_t retval = 0;

    left = x - r->xmin;
    top = y - r->ymin;
    bottom = r->ymax - y;
    right = r->xmax - x;

    closest = min(left, min(top, min(bottom, right)));

    if (left == closest)
	retval |= RCF_LEFT;
    else if (top == closest)
	retval |= RCF_TOP;
    else if (bottom == closest)
	retval |= RCF_BOTTOM;
    else if (right == closest)
	retval |= RCF_RIGHT;

    if (left < 0.20)
	retval |= RCF_LEFT;
    if (top < 0.20)
	retval |= RCF_TOP;
    if (bottom < 0.20)
	retval |= RCF_BOTTOM;
    if (right < 0.20)
	retval |= RCF_RIGHT;

    printf("closest %x\n", retval);
    
    return retval;
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
}
