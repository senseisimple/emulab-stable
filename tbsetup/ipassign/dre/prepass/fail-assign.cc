// fail-assign.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>
#include <string>

using namespace std;

int main()
{
    string line;
    getline(cin, line);
    getline(cin, line);
    cerr << "Failed. Aborting on $$" << endl
         << "$vertex=0$" << endl
         << "$edge=0$" << endl
         << "$vertex=googly$" << endl
         << "$vertex=-1$" << endl
         << "$edge=1000000$" << endl
         << "$once upon a time$" << endl
         << "$first$" << endl
         << "$nonsense=3$" << endl;
    return 1;

//    string line;
//    getline(cin, line);
//    while (cin)
//    {
//        cerr << line << endl;
//        getline(cin, line);
//    }
//    return 1;
}
