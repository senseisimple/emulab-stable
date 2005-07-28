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
 * Desc: Mezzanine IPC interface
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: mezz.h,v 1.3 2005-07-28 20:54:18 stack Exp $
 * Notes:
 *
 *  This library sets up a shared, memory-mapped object for exchanging
 *  data, and a signal for notifying other processes of updates to
 *  that object.  The mezzanine program will create the ipc interface;
 *  other processes will connect to an existing interface.
 *
 *  All units are SI unless otherwise noted; i.e. meters, radians,
 *  seconds.  Standard coordinate systems are used: positive x is to
 *  the right and positive y is upwards.
 *
 **************************************************************************/

#ifndef MEZZ_H
#define MEZZ_H

#include <sys/types.h>  // For pid_t
#include <stdint.h>     // For uint8_t, uint32_t, etc

// Some constants
#define MEZZ_MAX_AREA 768*576  // Maximum number of pixels in the image.
#define MEZZ_MAX_MASK    18    // Maximum number of points in the mask.
#define MEZZ_MAX_DEWARP  64    // Maximum number of dewarping calibration points.
#define MEZZ_MAX_CLASSES  4    // Maximum number of color classes we recognize.
#define MEZZ_MAX_BLOBS  100    // Maximum number of blobs we will extract.
#define MEZZ_MAX_OBJECTS 100   // Maximum number of extracted objects.


// Image mask info; used to identify the part of the image
// that corresponds to the area of interest.
typedef struct
{
  int poly[MEZZ_MAX_MASK][2];  // List of points defining a polygon.
} mezz_maskdef_t;


// Calibration info for dewarping
typedef struct
{
  int points;             // Number of dewarp points
  int ipos[MEZZ_MAX_DEWARP][2];    // The image coords of the calibration points.
  double wpos[MEZZ_MAX_DEWARP][2]; // The world coords of the calibration points.
  double iwtrans[2][8];   // Parameters for the image-to-world transform.
  double witrans[2][8];   // Parameters for the world-to-image transform.

  double warpFactor;
  double ocHeight;        // Height of the optical center of the lens in meters
  double scaleFactorX;
  double scaleFactorY;
  double ocX;
  double ocY;

  double gridX;
  double gridY;
} mezz_dewarpdef_t;


// Structure describing a color class.
// Used by the vision system for color classification.
typedef struct
{
  int color;        // Descriptve color (RGB32); used by gui.
  char name[64];    // Descriptive name for this color; used by gui.
  int vupoly[4][2]; // Polygon in VU components
  int yupoly[4][2]; // Polygon in YU components
  int vypoly[4][2]; // Polygon in VY components
} mezz_classdef_t;


// List of color class definitions
typedef struct
{
  int count;
  mezz_classdef_t classdefs[MEZZ_MAX_CLASSES];
} mezz_classdeflist_t;


// Options for the color blob extraction.
typedef struct
{
  int min_area, max_area;   // Allowed area (number of classified pixels).
  int min_sizex, max_sizex; // Allowed width.
  int min_sizey, max_sizey; // Allowed height
} mezz_blobdef_t;


// Information describing an extracted color blob.
typedef struct
{
  int class;        // What color class the blob belongs to.
  int min_x, max_x; // Bounding box for blob (image coords)
  int min_y, max_y; // Bounding box for blob (image coords)
  int area;         // Area of the blob (pixels).
  double ox, oy;    // Blob centroid (image coords).
  double wox, woy;  // Blob centroid (world coords).
  int object;       // Index of the object this blob is part of.
} mezz_blob_t;


// A list of blobs
typedef struct
{
  int count;
  mezz_blob_t blobs[MEZZ_MAX_BLOBS];
} mezz_bloblist_t;


// Structure describing an object in the image.
// We assume all objects are made up of two blobs.
typedef struct
{
  int valid;
  int class[2];       // Color class for the two blobs
  double max_disp;    // Maximum inter-frame displacement
  double max_sep;     // Maximum blob separation
  int max_missed;     // Max frames we can miss before we look for a new match.
  int missed;         // Number of missed frames (object not seen)
  double px, py, pa;  // Object pose (world cs).
    // additional info to make reporting blob pixel positions easier
    mezz_blob_t ablob;
    mezz_blob_t bblob;
} mezz_object_t;

/* // structure describing a "track" -- essentially, this just does a higher-level */
/* // track on the objectlist struct than does; it also handles pose (which  */
/* // ident_update does NOT) */

/* // if new, waiting for vmc-client to tell us who we are */
/* #define TRACK_STATUS_NEW        1 */
/* // if identified, everything's kosher, and we know what our id is. */
/* #define TRACK_STATUS_IDENTIFIED 2 */
/* // if invalid, this track is no longer valid and NO information should be read */
/* // from the enclosing struct -- it'll be bogus. */
/* #define TRACK_STATUS_INVALID    0 */

/* typedef struct { */
/*   // status can be NEW, IDENTIFIED, INVALID */
/*   int status; */
/*   int tracking_id; */
/*   double px, py, pa; */
/*   int frames_elapsed; */
  
/* } mezz_track_t; */

/* typedef struct { */
/*   int count; */
/*   mezz_track_t tracks[MEZZ_MAX_OBJECTS]; */
/*   double max_dist_delta; */
/*   double max_pose_delta; */
/* } mezz_tracklist_t; */


// Structure describing a list of objects.
typedef struct
{
  int count;
  mezz_object_t objects[MEZZ_MAX_OBJECTS];
} mezz_objectlist_t;
  

// Combined memory map data
typedef struct
{
  pid_t pids[32];       // List of pid's that should get events
  double time;          // The system time
  int count;            // Frame counter
  int calibrate;        // Non-zero if a calibration program is running
  int save;             // Non-zero if changes should be saved
  int width, height;    // Image dimensions (pixels)
  int area;             // Number of pixels in the image.
  int depth;            // Image depth (bits): 16, 24 or 32.
  mezz_maskdef_t maskdef;            // The image mask (identifies the field in the image).
  mezz_dewarpdef_t dewarpdef;        // Dewarping calibration info.
  mezz_classdeflist_t classdeflist;  // Color class definitions.
  mezz_blobdef_t blobdef;        // Blob definitions.
  uint32_t image[MEZZ_MAX_AREA]; // The raw frame data.
  uint8_t mask[MEZZ_MAX_AREA];   // The image mask.
  uint8_t class[MEZZ_MAX_AREA];  // The classified frame data.
  mezz_bloblist_t bloblist;      // The list of extracted blobs.
  mezz_objectlist_t objectlist;  // The list of identified objects.
/*   mezz_tracklist_t tracklist; */
} mezz_mmap_t;


// Initialise the interprocess comms.
// The server sets create to 1 to create the shared resources.
int mezz_init(int create,char *ipcfilepath);

// Finalize the ipc
// The server sets destroy to 1 to destroy the shared resources.
void mezz_term(int destroy);

// Get a pointer to the memory map
mezz_mmap_t *mezz_mmap();

// Raise an event (to be recieved by other processes)
void mezz_raise_event();

// Wait for an event (that was sent by another process)
int mezz_wait_event();

#endif
