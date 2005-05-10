// add-x.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>

using namespace std;

int main()
{
    double current = 0.0;
    double total = 0.0;
    cin >> current;
    while (cin)
    {
        cout << total << ' ' << current << endl;
        total += 0.01;
        cin >> current;
    }
    return 0;
}
