// route2dist.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>

using namespace std;

struct Lan
{
    Lan() : number(0) {}
    int number;
    vector<int> hosts;
    vector<string> ips;
};

int maxHost = 0;

void addToLan(Lan & theLan, string & streamString)
{
    istringstream stream(streamString);
    int temp;
    stream >> temp;
    int hostNumber = 0;
    string ip;
    stream >> hostNumber;
    stream >> ip;

    maxHost = max(maxHost, hostNumber);

    theLan.hosts.push_back(hostNumber);
    theLan.ips.push_back(ip);
}

int main()
{
    list<Lan> lanList;
    string cache;
    getline(cin, cache);
    int lanNumber = 0;
    stringstream cacheStream;
    if (cin)
    {
        cacheStream << cache;
        cacheStream >> lanNumber;
    }
    while (cin)
    {
        lanList.push_back(Lan());
        lanList.back().number = lanNumber;
//        addToLan(lanList.back(), cacheStream);

//        getline(cin, cache);
        while (cin && lanNumber == lanList.back().number)
        {
            addToLan(lanList.back(), cache);
            getline(cin, cache);
            if (cin)
            {
                istringstream stream(cache);
                stream >> lanNumber;
            }
        }
    }
    cerr << lanList.size() << endl;
    int lanCount = 0;
    list<Lan>::iterator countPos = lanList.begin();
    list<Lan>::iterator countLimit = lanList.end();
    for ( ; countPos != countLimit; ++countPos)
    {
        lanCount += (static_cast<int>(countPos->hosts.size())
                     * (static_cast<int>(countPos->hosts.size()) - 1)) >> 1;
    }
    cout << maxHost + 1 << " " << lanCount << endl;

    list<Lan>::iterator pos = lanList.begin();
    list<Lan>::iterator limit = lanList.end();
    for ( ; pos != limit; ++pos)
    {
        for (size_t i = 0; i < pos->hosts.size(); ++i)
        {
            for (size_t j = i + 1; j < pos->hosts.size(); ++j)
            {
                cout << pos->hosts[i] << " " << pos->ips[i] << " ";
                cout << pos->hosts[j] << " " << pos->ips[j] << " 1" << endl;
            }
        }
    }
    return 0;
}

