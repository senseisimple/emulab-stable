/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file ledManager.cc
 *
 * Implementation file for the ledManager class.
 */

#include "config.h"

#include <stdio.h>
#include <assert.h>

#include "ledManager.hh"

/**
 * The millisecond time ranges for each pattern.  The ranges are laid out so
 * that the state of the LED alternates between each range, with it initially
 * being turned on.
 *
 * @see find_range_start
 */
static unsigned long lm_pattern_ranges[][16] = {
    { 0 },

    /* LMP_BLANK */		{ 0 },	/* -ignored- */
    /* LMP_DOT */		{ 0, 500, 1000, 0 },
    /* LMP_FAST_DOT */		{ 0, 100, 200, 0 },
    /* LMP_DASH */		{ 0, 1000, 1500, 0 },
    /* LMP_DASH_DOT */		{ 0, 1000, 1250, 1500, 1750, 0 },
    /* LMP_DASH_DOT_DOT */	{ 0, 1000, 1250, 1500, 1750, 2000, 2250, 0 },
    /* LMP_LINE */		{ 0 },	/* -ignored- */

    { 0 },
};

char *ledClient::lm_pattern_names[LMP_MAX] = {
    "",
    
    "blank",
    "dot",
    "fast-dot",
    "dash",
    "dash-dot",
    "dash-dot-dot",
    "line"
};

ledClient::lm_pattern_t ledClient::findPattern(char *name)
{
    lm_pattern_t retval = LMP_MAX;
    int lpc;
    
    assert(name != NULL);

    for (lpc = LMP_MIN + 1; (lpc < LMP_MAX) && (retval == LMP_MAX); lpc++) {
	if (strcasecmp(name, lm_pattern_names[lpc]) == 0) {
	    retval = (lm_pattern_t)lpc;
	}
    }
    
    return retval;
}

ledClient::ledClient(int priority, lm_pattern_t lmp, unsigned long expiration)
    : lc_next(NULL),
      lc_priority(priority),
      lc_pattern(lmp),
      lc_expiration(expiration)
{
    assert(this->invariant());
}

ledClient::~ledClient()
{
}

ledManager::ledManager(acpGarcia &garcia, char *led_name)
    : lm_garcia(garcia),
      lm_led_name(led_name),
      lm_clients(NULL),
      lm_led_on(0),
      lm_first_tick(0)
{
    assert(led_name != NULL);
    assert(strlen(led_name) > 0);
}

ledManager::~ledManager()
{
}

bool ledManager::hasClient(ledClient *lc)
{
    bool retval = false;
    ledClient *curr_lc;

    curr_lc = this->lm_clients;
    while ((curr_lc != NULL) && (curr_lc != lc)) {
	curr_lc = curr_lc->lc_next;
    }

    if (curr_lc == lc) {
	retval = true;
    }
    
    return retval;
}

void ledManager::addClient(ledClient *lc)
{
    ledClient *curr_lc = this->lm_clients, **last_lc = &this->lm_clients;

    assert(lc != NULL);
    assert(lc->invariant());
    assert(!this->hasClient(lc));
    
    while (curr_lc != NULL && (lc->lc_priority < curr_lc->lc_priority)) {
	last_lc = &curr_lc->lc_next;
	curr_lc = curr_lc->lc_next;
    }

    lc->lc_next = *last_lc;
    *last_lc = lc;

    /* The head of the list was changed, need to force an update. */
    if (lc == this->lm_clients) {
	this->lm_last_range = -1;
	this->lm_first_tick = 0;
    }
}

void ledManager::remClient(ledClient *lc)
{
    ledClient *curr_lc = this->lm_clients, **last_lc = &this->lm_clients;

    assert(lc != NULL);
    assert(lc->invariant());

    /* The head of the list will change, need to force an update. */
    if (lc == this->lm_clients) {
	this->lm_last_range = -1;
	this->lm_first_tick = 0;
    }
    
    while ((curr_lc != NULL) && (lc != curr_lc)) {
	last_lc = &curr_lc->lc_next;
	curr_lc = curr_lc->lc_next;
    }

    if (curr_lc != NULL)
	*last_lc = lc->lc_next;
}

bool ledManager::update(unsigned long now)
{
    int new_led_on = this->lm_led_on.getIntVal();

    /* First, remove any stale clients, then */
    while ((this->lm_clients != NULL) &&
	   (this->lm_clients->lc_expiration != 0) &&
	   (now > this->lm_clients->lc_expiration)) {
	ledClient *expiredClient;

	expiredClient = this->lm_clients;
	this->remClient(expiredClient);
	
	delete expiredClient;
	expiredClient = NULL;
    }

    /* ... check if the head of the list was changed. */
    if (this->lm_first_tick == 0) {
	this->lm_first_tick = now;
	this->lm_led_on.set(0);
	this->lm_garcia.setNamedValue(this->lm_led_name, &this->lm_led_on);
    }

    if (this->lm_clients == NULL) {
	/* Nothing to do, turn off the LED. */
	new_led_on = 0;
	this->lm_first_tick = now;
    }
    else {
	switch (this->lm_clients->lc_pattern) {
	    /* Handle these patterns specially. */
	case ledClient::LMP_BLANK:
	    new_led_on = 0;
	    break;
	case ledClient::LMP_LINE:
	    new_led_on = 1;
	    break;
	    
	case ledClient::LMP_MIN:
	case ledClient::LMP_MAX:
	    assert(0);
	    break;
	    
	default:
	    {
		unsigned long *ranges;
		int start;

		/* Get the ranges for a pattern and */
		ranges = lm_pattern_ranges[this->lm_clients->lc_pattern];
		/* ... find the current range for right now. */
		start = find_range_start(ranges, now - this->lm_first_tick);
		if (start == -1) {
		    /* We're at the end of the range, need to restart. */
		    start = 0;
		    new_led_on = 1;
		    this->lm_first_tick = now;
		}
		else if (start != this->lm_last_range) {
		    /* The range is different, toggle the LED. */
		    new_led_on = !this->lm_led_on.getIntVal();
		}
		this->lm_last_range = start;
	    }
	    break;
	}
    }

    /* Check if we need to change the LED state. */
    if (new_led_on != this->lm_led_on.getIntVal()) {
	this->lm_led_on.set(new_led_on);
	this->lm_garcia.setNamedValue(this->lm_led_name, &this->lm_led_on);
    }

    return true;
}
