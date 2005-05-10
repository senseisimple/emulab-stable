// graph2single-source.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

int main()
{
    int bits = 0;
    int weight = 0;
    vector<int> hosts;
    string lineString;
    getline(cin, lineString);
    while (cin)
    {
        hosts.clear();
        istringstream line(lineString);
        int tempHost = 0;

        line >> bits >> weight;
        line >> tempHost;
        while(line)
        {
            hosts.push_back(tempHost);
            line >> tempHost;
        }

        for (size_t i = 0; i < hosts.size(); ++i)
        {
            for (size_t j = i + 1; j < hosts.size(); ++j)
            {
                cout << "I " << hosts[i] << " " << hosts[j] << " "
                     << static_cast<float>(weight) << endl;
            }
        }

        getline(cin, lineString);
    }

    cout << "C" << endl;
    return 0;
}
