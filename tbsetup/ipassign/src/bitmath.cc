// bitmath.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "bitmath.h"

using namespace std;

string ipToString(IPAddress ip)
{
    static unsigned int charMask = 0x000000FF;
    ostringstream buffer;
    for (int i = 3; i >= 0; --i)
    {
        buffer << ((ip >> i*8) & charMask);
        if (i > 0)
        {
            buffer << '.';
        }
    }
    return buffer.str();
}

IPAddress stringToIP(string const & source)
{
    size_t index = 0;
    bool success = true;
    IPAddress result = 0;
    for (size_t i = 0; i < 4 && success; ++i)
    {
        istringstream buffer(source.substr(index));
        IPAddress current = 0xf00;
        buffer >> current;
        if (current <= 0xff)
        {
            result = result | (current << ((3 - i)*8));
        }
        else
        {
            success = false;
        }
        index = source.find(".", index);
        // if a dot is on the last iteration, then no dot should
        // be found. Otherwise, there should be another dot.
        if (i < 3)
        {
            success = !(index == string::npos);
        }
        else
        {
            success = (index == string::npos);
        }
        ++index;
    }
    if (!success)
    {
        throw BadStringToIPConversionException(source);
    }
    return result;
}

unsigned int countToBlock(unsigned int count)
{
    return blockBitToBlock(countToBlockBit(count));
}

int countToBlockBit(unsigned int count)
{
    int size = 0;
    if (count >= 0)
    {
        for( ; count != 0; count >>= 1)
        {
            ++size;
        }
    }
    return size;
}

unsigned int blockBitToBlock(int blockBit)
{
    if (blockBit >= 0)
    {
        return 1 << blockBit;
    }
    else
    {
        return 0;
    }
}

IPAddress maskSizeToMask(int maskSize)
{
    return 0xFFFFFFFF << (32 - maskSize);
}




