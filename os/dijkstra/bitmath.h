// bitmath.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Utilities for dealing with ip numbers

#ifndef BITMATH_H_IP_ASSIGN_2
#define BITMATH_H_IP_ASSIGN_2

#include "Exception.h"

typedef unsigned int IPAddress;

class BadStringToIPConversionException : public StringException
{
public:
    explicit BadStringToIPConversionException(std::string const & error)
        : StringException("Bad String to IP conversion: " + error)
    {
    }
};

// take an unsigned int and produce a dotted quadruplet based on it.
// can throw bad_alloc
std::string ipToString(IPAddress ip);
IPAddress stringToIP(std::string const & source);

#endif
