// inet2graph.cc

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
    string headerString;
    getline(cin, headerString);
    if (cin)
    {
        istringstream header(headerString);
        size_t nodeCount = 0;
        string bufferString;
        header >> nodeCount;
        for (size_t i = 0; i < nodeCount; ++i)
        {
            getline(cin, bufferString);
        }
        getline(cin, bufferString);
        while (cin)
        {
            istringstream buffer(bufferString);
            size_t first = 0;
            size_t second = 0;
            size_t weight = 0;
            buffer >> first;
            buffer >> second;
            buffer >> weight;
            cout << "8 " << weight << ' ' << first << ' ' << second << endl;
            getline(cin, bufferString);
        }
    }
    return 0;
}
