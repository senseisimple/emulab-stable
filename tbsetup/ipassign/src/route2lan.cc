// route2lan.cc

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
    string bufferString;
    size_t currentLan = 0;
    bool firstTime = true;

    getline(cin, bufferString);
    while (cin)
    {
        istringstream buffer(bufferString);
        size_t lan = 0;
        size_t node = 0;
        buffer >> lan >> node;
        if (firstTime)
        {
            currentLan = lan;
            cout << node;
            firstTime = false;
        }
        else
        {
            if (lan == currentLan)
            {
                cout << ' ' << node;
            }
            else
            {
                currentLan = lan;
                cout << endl << node;
            }
        }
        getline(cin, bufferString);
    }
    return 0;
}
