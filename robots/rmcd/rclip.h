
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

    RCF_ALL = (RCF_TOP|RCF_BOTTOM|RCF_LEFT|RCF_RIGHT)
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

#define RC_CODE_STRING(x) ( \
    (x) == (RCF_TOP|RCF_LEFT) ? "tl" : \
    (x) == (RCF_TOP) ? "t" : \
    (x) == (RCF_TOP|RCF_RIGHT) ? "tr" : \
    (x) == (RCF_RIGHT) ? "r" : \
    (x) == (RCF_BOTTOM|RCF_RIGHT) ? "br" : \
    (x) == (RCF_BOTTOM) ? "b" : \
    (x) == (RCF_BOTTOM|RCF_LEFT) ? "bl" : \
    (x) == (RCF_LEFT) ? "l" : "u")

void rc_corner(rc_code_t rc, struct robot_position *rp, rc_rectangle_t r);
rc_code_t rc_compute_code(float x, float y, rc_rectangle_t r);
rc_code_t rc_compute_closest(float x, float y, rc_rectangle_t r);
rc_code_t rc_closest_corner(float x, float y, rc_rectangle_t r);
int rc_clip_line(rc_line_t line, rc_rectangle_t clip);

#endif
