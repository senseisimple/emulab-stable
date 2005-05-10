/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include "robotObject.h"

struct robot_object *roFindRobot(struct lnMinList *list, int id)
{
    struct robot_object *ro, *retval = NULL;

    assert(list != NULL);

    ro = (struct robot_object *)list->lh_Head;
    while ((ro->ro_link.ln_Succ != NULL) && (retval == NULL)) {
	if (ro->ro_id == id) {
	    retval = ro;
	}
	ro = (struct robot_object *)ro->ro_link.ln_Succ;
    }
    
    return retval;
}
