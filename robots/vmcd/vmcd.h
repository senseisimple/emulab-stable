/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _vmcd_h
#define _vmcd_h

#include "listNode.h"

struct vmc_client {
    mtp_handle_t vc_handle;
    struct lnMinList vc_frame;
    struct lnMinList vc_last_frame;
    unsigned long long vc_frame_count;
    char *vc_hostname;
    int vc_port;
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
