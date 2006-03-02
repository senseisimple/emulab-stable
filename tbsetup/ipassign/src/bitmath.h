// bitmath.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Utilities for dealing with ip numbers

#ifndef BITMATH_H_IP_ASSIGN_2
#define BITMATH_H_IP_ASSIGN_2

typedef unsigned int IPAddress;

#include "Exception.h"

class BadStringToIPConversionException : public StringException
{
public:
    explicit BadStringToIPConversionException(std::string const & error)
        : StringException("Bad String to IP conversion: " + error)
    {
    }
};

// A block is some binary number like 0x00010000 It is an ip address with
// all but one of its bits zero. This represents the number of hosts that
// can be in a particular subnet.

// A blockBit value is the number of bits inside of a particular block.
// For instance, in the example above, the blockBit value for that block
// is 16. The blockBit value of any block is log_2(Block).

// A count is the number of hosts that need to be contained by a Block.
// The blockBit value for a count is RoundUp(log_2(count)).

// take an unsigned int and produce a dotted quadruplet based on it.
// can throw bad_alloc
std::string ipToString(IPAddress ip);
IPAddress stringToIP(std::string const & source);

// take a count and return the smallest block that is large enough
// to contain count hosts.
// Cannot throw
unsigned int countToBlock(unsigned int count);

// take a count and return the number of bits in the smallest block
// large enough to contain count hosts.
// Cannot throw
int countToBlockBit(unsigned int count);

// take a blockBit value and return the associated block.
// Cannot throw
unsigned int blockBitToBlock(int blockBit);

IPAddress maskSizeToMask(int maskSize);

#endif
