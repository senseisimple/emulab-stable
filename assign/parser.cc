/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "parser.h"

#include <iostream>
#include <string>
using namespace std;

#ifdef DEBUG_PARSER
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

string_vector split_line(string line,char split_char)
{
  string_vector parsed;
  
  size_t prev_space, next_space;
  prev_space = 0;

  DEBUG(cerr << "Parsing line " << line << endl;)

  while ((next_space = line.find(' ',prev_space)) != string::npos) {
    if (next_space != prev_space) { // Skip multiple spaces
      parsed.push_back(line.substr(prev_space,(next_space - prev_space)).c_str());
      DEBUG(cerr << "Pushing string: " <<
              line.substr(prev_space,(next_space - prev_space)) << endl;)
    }
    prev_space = ++next_space;
  }
  if ((prev_space != next_space) && (prev_space < line.size()) &&
      (line[prev_space] != '\0')) {
    parsed.push_back(line.substr(prev_space,(next_space - prev_space)).c_str());
    DEBUG(cerr << "Pushing string: " <<
            line.substr(prev_space,(next_space - prev_space)) << endl;)
  }
  
  return parsed;
}

int split_two(string line,char split_char,string &a,string &b,string default_b)
{
  size_t space = line.find(split_char);
  if (space != string::npos) {
    a = line.substr(0,space).c_str();
    b = line.substr(++space,line.length() - space).c_str();
    return 0;
  } else {
    a = line.c_str();
    b = default_b;
    return 1;
  }
}

int split_two(string line,char split_char,string &a,string &b)
{
  return split_two(line,split_char,a,b,"");
}
