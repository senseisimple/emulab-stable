
#include "config.h"

#include <stdio.h>

#include "pilotHeartbeat.hh"

enum {
    LED_PRI_USER = 128,
    LED_PRI_MOVE = 0,
    LED_PRI_BATTERY = -128,
};

pilotHeartbeat::pilotHeartbeat()
{
    this->ph_button_manager.setCallback(this);
}

pilotHeartbeat::~pilotHeartbeat()
{
}

bool pilotHeartbeat::shortClick()
{
    
}

bool pilotHeartbeat::shortCommandClick()
{
    
}

bool pilotHeartbeat::longCommandClick()
{
    
}

bool pilotHeartbeat::update(acpGarcia &garcia, unsigned long now)
{
    this->ph_button_manager.updateUserButton(garcia, now);
    this->ph_led_manager.updateUserLED(garcia, now);
}
