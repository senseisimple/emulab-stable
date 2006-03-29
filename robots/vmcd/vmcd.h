/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _vmcd_h
#define _vmcd_h

#include "listNode.h"

struct vmc_client {
    /** Our connection to the vmc-client. */
    mtp_handle_t vc_handle;

    /** The current frame for this camera. */
    struct lnMinList vc_frame;
    /** The last frame received from this camera. */
    struct lnMinList vc_last_frame;
    /** Frame count for this camera so we can detect when they fall behind. */
    unsigned long long vc_frame_count;

    /** The host where the vmc-client is running. */
    char *vc_hostname;
    /** The port the vmc-client is listening on. */
    int vc_port;

    /**
     * The bounds for this camera.  These values are detected automatically as
     * objects move around in the field.
     */
    float vc_left;
    float vc_right;
    float vc_top;
    float vc_bottom;
};

/**
 * Version information.
 */
extern char build_info[];

#endif
