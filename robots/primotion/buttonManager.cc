/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file buttonManager.cc
 *
 * Implementation file for the buttonManager class.
 */

#include "config.h"

#include <stdio.h>

#include "garciaUtil.hh"
#include "buttonManager.hh"

/**
 * The millisecond time ranges for short and long clicks of the button.
 *
 * NOTE: We start at 500ms to allow for very quick clicks that can get the user
 * out of command-mode.
 *
 * @see find_range_start
 */
static unsigned long bm_click_range[] = {
    500, 2000,		/* Short click: .5s - 2s */
    4000, 10000,	/* Long click: 4s - 10s */
    0			/* -Sentinel- */
};

enum {
    BMR_SHORT_CLICK	= 0,
    BMR_LONG_CLICK	= 2
};

buttonCallback::~buttonCallback()
{
}

buttonManager::buttonManager(acpGarcia &garcia, char *button_name)
    : bm_garcia(garcia),
      bm_button_name(button_name),
      bm_callback(NULL),
      bm_command_mode(false),
      bm_button_on(0),
      bm_first_tick(0)
{
    assert(button_name != NULL);
    assert(strlen(button_name) > 0);
}

buttonManager::~buttonManager()
{
}

bool buttonManager::update(unsigned long now)
{
    bool retval = true;
    int button_on;

    assert(this->invariant());
    assert(this->bm_first_tick < now);
    
    /* Get the current state of the button. */
    button_on = this->bm_garcia.getNamedValue(this->bm_button_name)->
	getIntVal();

    if (!this->bm_button_on) {
	if (button_on) {
	    /* This is the first time we've seen the button pressed. */
	    this->bm_first_tick = now;
	}
    }
    else if (!button_on) {
	int start;

	/* The button has been released, so we find the range, and */
	start = find_range_start(bm_click_range, now - this->bm_first_tick);
	/* ... dispatch to the proper method. */
	if (!this->bm_command_mode) {
	    switch (start) {
	    case BMR_SHORT_CLICK:
		retval = this->bm_callback->shortClick(this->bm_garcia, now);
		break;
	    case BMR_LONG_CLICK:
		this->bm_command_mode = true;
		retval = this->bm_callback->
		    commandMode(this->bm_garcia, now, true);
		break;
	    }
	}
	else {
	    switch (start) {
	    case BMR_SHORT_CLICK:
		retval = this->bm_callback->
		    shortCommandClick(this->bm_garcia, now);
		break;
	    case BMR_LONG_CLICK:
		retval = this->bm_callback->
		    longCommandClick(this->bm_garcia, now);
		break;
	    }

	    /* NOTE: We always drop out of command mode after a click... */
	    this->bm_callback->commandMode(this->bm_garcia, now, false);
	    this->bm_command_mode = false;
	}

	this->bm_first_tick = 0;
    }

    this->bm_button_on = button_on;

    return retval;
}
