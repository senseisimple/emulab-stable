/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "parser.h"

#include <iostream>

#ifdef DEBUG_PARSER
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

string_vector split_line(crope line,char split_char)
{
  string_vector parsed;
  crope::const_iterator next_space,prev_space;
  prev_space = line.begin();

  DEBUG(cerr << "Parsing line " << line << endl;)

  while ((next_space = find(prev_space,line.end(),' ')) != line.end()) {
    if (next_space != prev_space) { // Skip multiple spaces
      parsed.push_back(line.substr(prev_space,next_space));
      DEBUG(cerr << "Pushing string: " << line.substr(prev_space,next_space) <<
	  endl;)
    }
    prev_space = ++next_space;
  }
  if (prev_space != next_space) {
    parsed.push_back(line.substr(prev_space,next_space));
    DEBUG(cerr << "Pushing string: " << line.substr(prev_space,next_space) <<
	endl;)
  }
  
  return parsed;
}

int split_two(crope line,char split_char,crope &a,crope &b,crope default_b)
{
  crope::const_iterator space = find(line.begin(),line.end(),split_char);
  a = line.substr(line.begin(),space);
  if (space != line.end()) {
    b = line.substr(++space,line.end());
    return 0;
  } else {
    b = default_b;
    return 1;
  }
}

int split_two(crope line,char split_char,crope &a,crope &b)
{
  return split_two(line,split_char,a,b,"");
}
