
#ifndef _pilot_heartbeat_hh
#define _pilot_heartbeat_hh

#include <stdio.h>

#include "acpGarcia.h"

#include "ledManager.hh"
#include "buttonManager.hh"

class pilotHeartbeat
{

public:
    
    pilotHeartbeat();
    virtual ~pilotHeartbeat();
    
    virtual bool shortClick();
    virtual bool shortCommandClick();
    virtual bool longCommandClick();

    bool update(acpGarcia &garcia, unsigned long now);

private:

    ledManager ph_led_manager;
    buttonManager ph_button_manager;
    
};

#endif
