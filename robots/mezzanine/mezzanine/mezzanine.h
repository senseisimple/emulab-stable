/* 
 *  Mezzanine - an overhead visual object tracker.
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Public strutures, functions
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: mezzanine.h,v 1.1 2004-12-12 23:36:33 johnsond Exp $
 ***************************************************************************/

#ifndef MEZZANINE_H
#define MEZZANINE_H

#include "mezz.h"

/***************************************************************************
 * Frame grabber
 ***************************************************************************/

// Initialise the frame grabber
int fgrab_init(mezz_mmap_t *mmap);

// Close the frame grabber
void fgrab_term();

// Get the next frame (blocking)
int *fgrab_get();


/***************************************************************************
 * Color classifier
 ***************************************************************************/

// Initialise the classifier
int classify_init(mezz_mmap_t *mmap);

// Classify the frame.
// Returns an image in which each pixel denotes the class.
// We return it as a byte to reduce memory through-put.
uint8_t *classify_update(int *frame);

// Close the classifier
void classify_term();


/***************************************************************************
 * Blob finder
 ***************************************************************************/

// Initialise the blob finder
int blobfind_init(mezz_mmap_t *mmap);

// Extract blobs from a color-classified image
// Returns a list of blobs
mezz_bloblist_t *blobfind_update(uint8_t *class);

// Close the blob finder
void blobfind_term();


/***************************************************************************
 * Dewarper: computes world pose of blobs
 ***************************************************************************/

// Initialise the dewarper
int dewarp_init(mezz_mmap_t *mmap);

// Close the dewarper
void dewarp_term();

// Compute the world pose of all the blobs
mezz_bloblist_t *dewarp_update(mezz_bloblist_t *bloblist);


/***************************************************************************
 * Identifier: identifies our robots, opposition robots, the ball.
 ***************************************************************************/

// Initialise the identer
int ident_init(mezz_mmap_t *mmap);

// Close the identer
void ident_term();

// Ident identities to blobs
mezz_objectlist_t *ident_update(mezz_bloblist_t *bloblist);


/***************************************************************************
 * Track objects
 ***************************************************************************/

// Initialise the tracker
int track_init(mezz_mmap_t *mmap);

// Close the tracker
void track_term();

// Update tracks
void track_update(mezz_objectlist_t *objectlist);


/***************************************************************************
 * Misc useful stuff
 ***************************************************************************/

// Determine number of elements in a static array
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))


#endif
