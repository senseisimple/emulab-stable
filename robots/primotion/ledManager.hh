/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file ledManager.hh
 *
 * Header file for the ledManager class.
 */

#ifndef _led_manager_hh
#define _led_manager_hh

#include <assert.h>

#include "acpGarcia.h"

#include "garciaUtil.hh"

/**
 * Helper class that encodes an LED pattern and priority.
 */
class ledClient
{

public:

    /**
     * Enumeration of possible patterns to display with the LED.
     */
    typedef enum {
	LMP_MIN,		/*< Minimum value. */
	
	LMP_BLANK,		/*< "     " */
	LMP_DOT,		/*< ". . ." */
	LMP_FAST_DOT,		/*< "....." */
	LMP_DASH,		/*< "- - -" */
	LMP_DASH_DOT,		/*< "- . -" */
	LMP_DASH_DOT_DOT,	/*< "- . ." */
	LMP_LINE,		/*< "-----" */

	LMP_MAX			/*< Maximum value. */
    } lm_pattern_t;

    /**
     * Human readable names for each of the patterns, indexed by the
     * lm_pattern_t enumeration.
     */
    static char *lm_pattern_names[LMP_MAX];

    /**
     * Find an LED pattern that matches the given name.
     *
     * @param name The human readable name of the pattern, must match one of
     * the values in lm_pattern_names.'
     * @return The enumerated value of the patter or LMP_MAX if no match was
     * found.
     */
    static lm_pattern_t findPattern(char *name);

    /**
     * Constructs an ledClient object with the given values.
     *
     * @param priority The priority of this client, higher numbers will be
     * given preference.
     * @param lmp The pattern to display on the LED.
     * @param expiration The expiration time for this client, given in
     * milliseconds.  Or, zero if the pattern should be displayed forever.
     */
    ledClient(int priority, lm_pattern_t lmp, unsigned long expiration = 0);

    /**
     * Deconstruct the object.
     */
    virtual ~ledClient();

    /**
     * Check the object to make sure it is internally consistent.
     *
     * @return true
     */
    bool invariant()
    {
	assert(this->lc_pattern > LMP_MIN);
	assert(this->lc_pattern < LMP_MAX);

	return true;
    };

    /**
     * Pointer to the next client in a singly linked list.
     */
    ledClient *lc_next;

    /**
     * The priority of this client.
     */
    int lc_priority;

    /**
     * The pattern this client should display on the LED.
     */
    lm_pattern_t lc_pattern;

    /**
     * The expiration time for this client, given in milliseconds.  Or, zero if
     * the pattern should be displayed forever.
     */
    unsigned long lc_expiration;
    
};
    
/**
 * Arbiter for an LED on the garcia robot.  Because of the low-bandwidth nature
 * of the device and the need for many things to communicate something, the
 * manager can display multiple patterns and decides which pattern to display
 * based on a simple priority queue.
 */
class ledManager : public pollCallback
{

public:

    /**
     * Construct an ledManager object for a particular LED on a garcia.
     *
     * @param garcia The garcia the LED is located on.
     * @param led_name The name of the LED property in the garcia object
     * (e.g. "user-led").
     */
    ledManager(acpGarcia &garcia, char *led_name);
    
    /**
     * Deconstruct this object.
     */
    virtual ~ledManager();

    /**
     * Add a client to the manager.  If this new client has a higher priority
     * than any existing clients, its pattern will be displayed instead of the
     * currently active one.  If this client has an expiration time, the client
     * will automatically be removed and deleted when that time has passed.
     *
     * @param lc The client to add to the manager.
     */
    void addClient(ledClient *lc);
    
    /**
     * Remove a client from the manager.  If this client has the highest
     * priority, the displayed pattern will change to the new highest priority.
     * If there are no more clients being displayed, the LED will be turned
     * off.
     *
     * @param lc The client to add remove from the manager.
     */
    void remClient(ledClient *lc);

    bool hasClient(ledClient *lc);

    /**
     * Update the LED property to match the currently active pattern.
     *
     * @param now The current time in milliseconds.
     * @return true
     */
    bool update(unsigned long now);

private:

    /**
     * The garcia where the LED is located.
     */
    acpGarcia &lm_garcia;
    
    /**
     * The name of the LED property on the garcia object (e.g. "user-led").
     */
    char *lm_led_name;

    /**
     * A singly linked list of clients, ordered from highest to lowest
     * priority.
     */
    ledClient *lm_clients;

    /**
     * Value object used to turn the LED on and off.
     */
    acpValue lm_led_on;

    /**
     * The last time range the manager displayed on the LED.
     *
     * @see lm_pattern_ranges
     */
    int lm_last_range;

    /**
     * The first tick that the start of a pattern was displayed.  This value
     * will be increased as the pattern loops endlessly.
     *
     * @see update
     * @see lm_pattern_ranges
     */
    unsigned long lm_first_tick;
    
};

#endif
