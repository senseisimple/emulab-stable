// brite2graph.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int main()
{
    string bufferString;
    getline(cin, bufferString);
    while (cin && bufferString.substr(0, 5) != "Edges")
    {
        getline(cin, bufferString);
    }

    getline(cin, bufferString);
    while (cin)
    {
        size_t temp = 0;
        size_t source = 0;
        size_t dest = 0;
        istringstream buffer(bufferString);
        buffer >> temp >> source >> dest;
        cout << "2 1 " << source << ' ' << dest << endl;
        getline(cin, bufferString);
    }

    return 0;
}
