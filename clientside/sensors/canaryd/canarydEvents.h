/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file canarydEvents.h
 *
 * Header file for the event related stuff for canaryd.
 */

#ifndef CANARYD_EVENTS_H
#define CANARYD_EVENTS_H

#include "event.h"
#include "tbdefs.h"

/**
 * Initialize the canaryd connection to the Emulab event system.
 *
 * @param event_server An "elvin://" URL for the server.
 * @return True if the initialization was successful, false otherwise.
 */
int ceInitCanarydEvents(const char *event_server);

/**
 * The path to the event secret key file.
 */
#define EVENTKEYFILE "/var/emulab/boot/eventkey"

/*
 * Global data for the canaryd-related event stuff.
 *
 * ced_PidEid - The pid/eid of the experiment.  For example, "tbres/ftest".
 * ced_Handle - The handle to the event system.
 */
struct ceCanarydEventsData {
    const char *ced_PidEid;
    event_handle_t ced_Handle;
} canaryd_events_data;

/**
 * Callback for individual events.
 *
 * @param handle The event handle the event was received on.
 * @param notification The event notification itself.
 * @param data ...
 */
extern void ceEventCallback(event_handle_t handle,
			    event_notification_t notification, 
			    void *data);

#endif
