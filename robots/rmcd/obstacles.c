
#include "config.h"

#include <float.h>
#include <stdio.h>
#include <assert.h>

#include "rclip.h"
#include "obstacles.h"

#define min(x, y) ((x) < (y)) ? (x) : (y)
#define max(x, y) ((x) > (y)) ? (x) : (y)

int ob_find_obstacle(struct obstacle_config *oc_out,
		     struct mtp_config_rmc *mcr,
		     rc_line_t line)
{
    int lpc, retval = 0;

    assert(oc_out != NULL);
    assert(mcr != NULL);
    assert(line != NULL);

    oc_out->xmin = oc_out->ymin = FLT_MAX;
    oc_out->xmax = oc_out->ymax = 0;
    for (lpc = 0; lpc < mcr->obstacles.obstacles_len; lpc++) {
	struct rc_line line_cp = *line;
	struct obstacle_config *oc;
	
	oc = &mcr->obstacles.obstacles_val[lpc];
	printf(" %f %f -> %f %f  --  %f %f %f %f\n",
	       line->x0, line->y0,
	       line->x1, line->y1,
	       oc->xmin, oc->ymin,
	       oc->xmax, oc->ymax);
	if (rc_clip_line(&line_cp, oc)) {
	    oc_out->xmin = min(oc_out->xmin, oc->xmin);
	    oc_out->ymin = min(oc_out->ymin, oc->ymin);
	    oc_out->xmax = max(oc_out->xmax, oc->xmax);
	    oc_out->ymax = max(oc_out->ymax, oc->ymax);
	    
	    retval = 1;
	}
    }

    return retval;
}
