
#include "config.h"

#include <stdio.h>
#include <assert.h>

#include "rclip.h"
#include "obstacles.h"

struct obstacle_config *ob_find_obstacle(struct mtp_config_rmc *mcr,
					 rc_line_t line)
{
    struct obstacle_config *retval = NULL;
    int lpc;

    assert(mcr != NULL);
    
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
	    retval = oc;
	    break;
	}
    }

    return retval;
}
