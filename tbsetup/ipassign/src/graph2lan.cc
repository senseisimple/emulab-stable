// graph2lan.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main()
{
    string bufferSTring;

    getline(cin, bufferString);
    while (cin)
    {
        istringstream buffer(bufferString);
        size_t temp;
        // ignore weight and minBits
        buffer >> temp;
        buffer >> temp;

        buffer >> temp;
        if (buffer)
        {
            cout << temp;
        }

        buffer >> temp;
        while (buffer)
        {
            cout << ' ' << temp
            buffer >> temp;
        }
        cout << endl;
        getline(cin, bufferString);
    }
    return 0;
}
