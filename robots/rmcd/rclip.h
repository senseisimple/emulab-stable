/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _rclip_h
#define _rclip_h

/**
 * @file rclip.h
 *
 * Line clipping and other tools used to detect when a straight line path will
 * intersect with an obstacle.
 *
 * I just stole this from some publicly available graphics code.
 */

#include "mtp.h"

enum {
    RCB_TOP,
    RCB_BOTTOM,
    RCB_RIGHT,
    RCB_LEFT,
};

/**
 * These flags represent the four sides of a rectangle.  They are used in
 * slightly different ways by each of the functions.  For example, rc_corner
 * uses them to represent corners of the rectangle.
 */
enum {
    RCF_TOP = (1L << RCB_TOP),
    RCF_BOTTOM = (1L << RCB_BOTTOM),
    RCF_RIGHT = (1L << RCB_RIGHT),
    RCF_LEFT = (1L << RCB_LEFT),

    RCF_ALL = (RCF_TOP|RCF_BOTTOM|RCF_LEFT|RCF_RIGHT)
};

/**
 * Type used for storing RCF_ values.
 */
typedef unsigned long rc_code_t;

/**
 * The rectangle we'll be operating on is just a regular obstacle configuration
 * from MTP.
 */
typedef struct obstacle_config *rc_rectangle_t;

/**
 * Structure used to describe a line segment.
 */
typedef struct rc_line {
    float x0;
    float y0;
    float x1;
    float y1;
} *rc_line_t;

/**
 * Any points being operated on are the same as robot_positions from MTP.
 */
typedef struct robot_position *rc_point_t;

/**
 * Convert an rc_code_t value that represents a single side or corner into a
 * human readable string.
 *
 * @param x The rc_code_t value to convert.
 * @return A static string that represents the given value.
 */
#define RC_CODE_STRING(x) ( \
    (x) == (RCF_TOP|RCF_LEFT) ? "tl" : \
    (x) == (RCF_TOP) ? "t" : \
    (x) == (RCF_TOP|RCF_RIGHT) ? "tr" : \
    (x) == (RCF_RIGHT) ? "r" : \
    (x) == (RCF_BOTTOM|RCF_RIGHT) ? "br" : \
    (x) == (RCF_BOTTOM) ? "b" : \
    (x) == (RCF_BOTTOM|RCF_LEFT) ? "bl" : \
    (x) == (RCF_LEFT) ? "l" : "u")

rc_code_t rc_compute_code(float x, float y, rc_rectangle_t r);

/**
 * Perform clipping on the line segment with the given rectangle.
 *
 * @param line_inout The line segment to clip.  The structure will be updated
 * to reflect the part of the line that falls within the clipping rectangle.
 * @param clip The clipping rectangle.  Any part of the line that is outside
 * this rectangle will be chopped off.
 * @return True if the line falls within the clipping rectangle, false
 * otherwise.
 */
int rc_clip_line(rc_line_t line_inout, rc_rectangle_t clip);

/**
 * Check if two rectangles intersect.
 *
 * @param r1 The first rectangle.
 * @param r2 The second rectangle.
 * @return True if the two rectangles intersect, false otherwise.
 */
int rc_rect_intersects(rc_rectangle_t r1, rc_rectangle_t r2);

/**
 * Set a point structure to the value of a corner of the given rectangle.
 *
 * @param rc The corner of the rectangle to select
 * @param rp_out The point to set.
 * @param r The rectangle to get the corner coordinates from.
 * @return rp
 */
rc_point_t rc_corner(rc_code_t rc, rc_point_t rp_out, rc_rectangle_t r);

#endif
