
#ifndef _rclip_h
#define _rclip_h

#include "mtp.h"

/*
 * define the coding of end points
 */
enum {
    RCB_TOP,
    RCB_BOTTOM,
    RCB_RIGHT,
    RCB_LEFT,
};

enum {
    RCF_TOP = (1L << RCB_TOP),
    RCF_BOTTOM = (1L << RCB_BOTTOM),
    RCF_RIGHT = (1L << RCB_RIGHT),
    RCF_LEFT = (1L << RCB_LEFT),
};

typedef unsigned long rc_code_t;

/*
 * a rectangle is defined by 4 sides
 */
typedef struct obstacle_config *rc_rectangle_t;

typedef struct rc_line {
    float x0;
    float y0;
    float x1;
    float y1;
} *rc_line_t;

rc_code_t rc_compute_code(float x, float y, rc_rectangle_t r);
rc_code_t rc_compute_closest(float x, float y, rc_rectangle_t r);
int rc_clip_line(rc_line_t line, rc_rectangle_t clip);

#endif
