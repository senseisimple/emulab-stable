/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __PARSER_H
#define __PARSER_H

typedef vector<crope> string_vector;

int split_two(crope line,char split_char,crope &a,crope &b);
int split_two(crope line,char split_char,crope &a,crope &b,crope default_b);
string_vector split_line(crope line,char split_char);


#endif
