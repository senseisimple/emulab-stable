// Compressor.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Compressor.h"

using namespace std;

const int Compressor::shift[PRIVATE_SUBNET_COUNT] = {IP_SIZE - 8,
                                                     IP_SIZE - 12,
                                                     IP_SIZE - 16};

const IPAddress Compressor::prefix[PRIVATE_SUBNET_COUNT] = {10,
                                                            (172 << 8) + 16,
                                                            (192 << 8) + 168};

Compressor::PRIVATE_SUBNET Compressor::whichSubnet(IPAddress destIp)
{
#ifdef ROCKETFUEL_TEST
    return SUB_10;
#else
    for (PRIVATE_SUBNET i = PRIVATE_SUBNET_MIN; i < PRIVATE_SUBNET_COUNT;
         i = static_cast<PRIVATE_SUBNET>(i + PRIVATE_SUBNET_UNIT))
    {
        if ((destIp >> shift[i]) == prefix[i])
        {
            return i;
        }
    }
    return PRIVATE_SUBNET_INVALID;
#endif
}
