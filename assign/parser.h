/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __PARSER_H
#define __PARSER_H

#include "port.h"
#include <string>
using namespace std;

#include <vector>
using namespace std;

typedef vector<string> string_vector;

int split_two(string line,char split_char,string &a,string &b);
int split_two(string line,char split_char,string &a,string &b,string default_b);
string_vector split_line(string line,char split_char);

#endif
