
#ifndef _vmcd_h
#define _vmcd_h

#include "listNode.h"

struct vmc_client {
    mtp_handle_t vc_handle;
    struct lnMinList vc_frame;
    char *vc_hostname;
    int vc_port;
    float vc_left;
    float vc_right;
    float vc_top;
    float vc_bottom;
};

#endif
