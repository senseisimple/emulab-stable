// top2graph.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// top2graph is a utility which converts the .top files used for assign into
// .graph files used for ipassign. There is a single optional flag '-m'. If
// this flag is set, the mapping from assign names to ipassign numbers and
// the mapping from ipassign numbers to assign names is sent to the standard
// error in addition to the conversion being sent to the standard output.

#include <iostream>
#include <map>
#include <list>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

int main(int argc, char * argv[])
{
    bool lan = false;
    bool lineBegin = true;
    string stringBuffer;
    int counter = 0;
    map<string, int> nodeMap;
    map< string, list<int> > lanMap;

    getline(cin, stringBuffer);
    while (cin)
    {
        istringstream buffer(stringBuffer);
        string type;
        buffer >> type;
        if (type == "node")
        {
            string name, type;
            buffer >> name >> type;
            if (type == "lan")
            {
                lanMap[name];
            }
            else
            {
                nodeMap[name] = counter;
                ++counter;
            }
        }
        else if (type == "link")
        {
            string temp, first, second;
            buffer >> temp >> first >> second;
            // first is a lan and second is a node
            if (nodeMap.find(first) == nodeMap.end())
            {
                lanMap[first].push_back(nodeMap[second]);
            }
            // first is a node and second is a lan
            else if (nodeMap.find(second) == nodeMap.end())
            {
                lanMap[second].push_back(nodeMap[first]);
            }
            // both are nodes
            else
            {
                lanMap["top2graph-LinkLan-" + first + second]
                    .push_back(nodeMap[first]);
                lanMap["top2graph-LinkLan-" + first + second]
                    .push_back(nodeMap[second]);
            }
        }
        getline(cin, stringBuffer);
    }
    map< string, list<int> >::iterator pos = lanMap.begin();
    for ( ; pos != lanMap.end(); ++pos)
    {
        if (!(pos->second.empty()))
        {
            cout << "1 ";
        }
        list<int>::iterator lanPos = pos->second.begin();
        for ( ; lanPos != pos->second.end(); ++lanPos)
        {
            cout << *lanPos << " ";
        }
        if (!(pos->second.empty()))
        {
            cout << endl;
        }
    }
    if (argc == 2 && argv[1] == string("-m"))
    {
        multimap<int, string> reverseNodeMap;
        map<string, int>::iterator nodeMapIterator = nodeMap.begin();
        for ( ; nodeMapIterator != nodeMap.end(); ++nodeMapIterator)
        {
            reverseNodeMap.insert(make_pair(nodeMapIterator->second,
                                            nodeMapIterator->first));
            cerr <<  setw(20) << nodeMapIterator->first
                 << " --> " << nodeMapIterator->second << endl;
        }
        multimap<int, string>::iterator reverseIterator;
        reverseIterator = reverseNodeMap.begin();
        for ( ; reverseIterator != reverseNodeMap.end(); ++reverseIterator)
        {
            cerr << setw(20) <<  reverseIterator->first
                 << " --> " << reverseIterator->second << endl;
        }
    }
}



