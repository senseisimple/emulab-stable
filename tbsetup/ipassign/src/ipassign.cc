// ipassign.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// For command line arguments, see README
// This program processes from stdin to stdout.
// Input and output specifications can be found in README

#include "lib.h"
#include "Exception.h"
#include "Framework.h"

using namespace std;

// constants
const int totalBits = 32;
const int prefixBits = 8;
const int postfixBits = totalBits - prefixBits;
const IPAddress prefix(10 << postfixBits);
const IPAddress prefixMask(0xFFFFFFFF << postfixBits);
const IPAddress postfixMask(0xFFFFFFFF >> prefixBits);

void usage(ostream & output);

int main(int argc, char * argv[])
{
    int errorCode = 0;

    try
    {
        Framework frame(argc, argv);
        frame.input(cin);
        frame.ipAssign();
        frame.printIP(std::cout);
        frame.route();
        frame.printRoute(std::cout);
    }
    catch (InvalidArgumentException const & error)
    {
        std::cerr << "ipassign: " << error.what() << std::endl;
        usage(std::cerr);
    }
    catch (std::exception const & error)
    {
        std::cerr << "ipassign: " << error.what() << std::endl;
        errorCode = 1;
    }

    return errorCode;
}

void usage(ostream & output)
{
    output << "Usage: ipassign [-p#] [-sbg] [-hln] [-tvc]" << endl;
}



