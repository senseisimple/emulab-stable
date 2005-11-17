/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file be_user.h
 */

#ifndef _be_user_h
#define _be_user_h

#ifdef __cplusplus
extern "C" {
#endif

/* Flip to the user, but only if we are currently root. */
int be_user(const char *username);

#ifdef __cplusplus
}
#endif

#endif
