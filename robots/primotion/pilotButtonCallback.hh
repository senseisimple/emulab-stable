/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file pilotButtonCallback.hh
 *
 * Header file for the pilotButtonCallback class.
 */

#ifndef _pilot_button_callback_hh
#define _pilot_button_callback_hh

#include "dashboard.hh"
#include "wheelManager.hh"

/**
 * User-button callback class for the garcia-pilot tool.
 */
class pilotButtonCallback :
    public buttonCallback
{

public:

    /**
     * Construct a button callback and automatically add it to the given
     * dashboard.
     *
     * @param db The dashboard to use for changing the user LED and receiving
     * user button events.
     * @param wm The wheelManager to use when moving the robot in a test
     * sequence.
     */
    pilotButtonCallback(dashboard &db, wheelManager &wm);

    /**
     * Destructor
     */
    virtual ~pilotButtonCallback();    

    /**
     * Method called when the user clicks the button for a short time, which
     * triggers the robot to run through a test movement sequence.
     *
     * @param garcia The garcia object to manipulate.
     * @param now The current time in milliseconds.
     * @return true
     */
    virtual bool shortClick(acpGarcia &garcia, unsigned long now);
    
    /**
     * Method called when the user initiates "command mode", which causes the
     * the LED to display the pattern, LED_PATTERN_COMMAND_MODE.
     *
     * @param garcia The garcia object to manipulate.
     * @param now The current time in milliseconds.
     * @param on True when entering command mode and false when exiting.
     * @return true
     */
    virtual bool commandMode(acpGarcia &garcia, unsigned long now, bool on);

    /**
     * Method called when the user click the button for a short time while in
     * command mode, which triggers a reboot and causes the daemon to exit.
     *
     * @param garcia The garcia object to manipulate.
     * @param now The current time in milliseconds.
     * @return false
     */
    virtual bool shortCommandClick(acpGarcia &garcia, unsigned long now);
    
    /**
     * Method called when the user click the button for a short time while in
     * command mode, which triggers a shutdown and causes the daemon to exit.
     *
     * @param garcia The garcia object to manipulate.
     * @param now The current time in milliseconds.
     * @return false
     */
    virtual bool longCommandClick(acpGarcia &garcia, unsigned long now);

private:

    /**
     * The dashboard to use for changing the user LED and receiving user button
     * events.
     */
    dashboard &pbc_dashboard;

    /**
     * The wheelManager to use when moving the robot in a test sequence.
     */
    wheelManager &pbc_wheel_manager;

    /**
     * The ledClient used to indicate when the robot is in command mode.
     */
    ledClient pbc_command_notice;
    
};

#endif
