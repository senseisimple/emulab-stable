/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file buttonManager.hh
 *
 * Header file for the buttonManager class.
 */

#ifndef _button_manager_hh
#define _button_manager_hh

#include <assert.h>
#include <string.h>

#include "acpGarcia.h"

#include "garciaUtil.hh"

/**
 * Base class for button callbacks.
 */
class buttonCallback
{
    
public:

    /**
     * Destructor.
     */
    virtual ~buttonCallback();

    /**
     * Method called when the button is hold down for a short time.
     *
     * @param garcia The garcia the button is located on.
     * @param now The current time in milliseconds.
     * @return True if the main update loop should continue.
     */
    virtual bool shortClick(acpGarcia &garcia, unsigned long now)
    { return true; };

    /**
     * Method called when switching in and out of "command" mode.
     *
     * @param garcia The garcia the button is located on.
     * @param now The current time in milliseconds.
     * @param on True if in "command" mode, false otherwise.
     * @return True if the main update loop should continue.
     */
    virtual bool commandMode(acpGarcia &garcia, unsigned long now, bool on)
    { return true; };

    /**
     * Method called when the button is hold down for a short time in "command"
     * mode.
     *
     * @param garcia The garcia the button is located on.
     * @param now The current time in milliseconds.
     * @return True if the main update loop should continue.
     */
    virtual bool shortCommandClick(acpGarcia &garcia, unsigned long now)
    { return true; };

    /**
     * Method called when the button is hold down for a long time in "command"
     * mode.
     *
     * @param garcia The garcia the button is located on.
     * @param now The current time in milliseconds.
     * @return True if the main update loop should continue.
     */
    virtual bool longCommandClick(acpGarcia &garcia, unsigned long now)
    { return true; };

};

/**
 * Arbiter for a button on the garcia robot.  The class will detect long and
 * short clicks of the button and call the appropriate user-defined callback.
 * To expand the number of choices, an initial long click will put the class in
 * "command" mode and giving the user another pair of choices.
 *
 * @see bm_click_range
 */
class buttonManager : public pollCallback
{

public:

    /**
     * Construct a buttonManager object for a particular button on a garcia.
     *
     * @param garcia The garcia the button is located on.
     * @param button_name The name of the button property in the garcia object
     * (e.g. "user-button").
     */
    buttonManager(acpGarcia &garcia, char *button_name);

    /**
     * Deconstruct this object.
     */
    virtual ~buttonManager();

    /**
     * @param bc The callback that is responsible for handling button clicks.
     */
    void setCallback(buttonCallback *bc) { this->bm_callback = bc; };

    /**
     * Update the manager's state by polling the button property.
     *
     * @param now The current time in milliseconds.
     */
    bool update(unsigned long now);

    /**
     * Check the object to make sure it is internally consistent.
     *
     * @return true
     */
    bool invariant()
    {
	assert(this->bm_button_name != NULL);
	assert(strlen(this->bm_button_name) > 0);
	assert(this->bm_callback != NULL);
	
	return true;
    };
    
private:

    /**
     * The garcia where the button is located.
     */
    acpGarcia &bm_garcia;

    /**
     * The name of the button property on the garcia object
     * (e.g. "user-button").
     */
    char *bm_button_name;

    /**
     * The callback responsible for handling button clicks.
     */
    buttonCallback *bm_callback;

    /**
     * True if the manager is in "command" mode and waiting for another click,
     * false otherwise.
     */
    bool bm_command_mode;

    /**
     * The last state of the button.
     */
    int bm_button_on;

    /**
     * The first tick that the button was held down.
     */
    unsigned long bm_first_tick;
    
};

#endif
