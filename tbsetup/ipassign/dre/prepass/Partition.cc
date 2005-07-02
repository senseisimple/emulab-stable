// Partition.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "prepass.h"
#include "Partition.h"
#include "coprocess.h"

//-----------------------------------------------------------------------------

Partition::Partition(int newNumber)
    : address(0)
    , isTree(false)
    , lanCount(0)
    , hostCount(0)
{
    setNumber(newNumber);
}

//-----------------------------------------------------------------------------

void Partition::setNumber(int newNumber)
{
    // TODO: Assign a real address. Not just the partition number.
//    address = newNumber;
    number = newNumber;
}

//-----------------------------------------------------------------------------

int Partition::getNumber(void) const
{
    return number;
}

//-----------------------------------------------------------------------------

void Partition::setAddress(int newAddress)
{
    address = newAddress;
}

//-----------------------------------------------------------------------------

int Partition::getAddress(void) const
{
    return address;
}

//-----------------------------------------------------------------------------

void Partition::addLan(int lanNumber)
{
    // Have we already added this LAN in the past?
    map<int, int>::iterator lanSearch = lanToOrder.find(lanNumber);
    if (lanSearch == lanToOrder.end())
    {
        // If not, add a mapping for the LAN.
        lanToOrder[lanNumber] = lanCount;
        orderToLan[lanCount] = lanNumber;
        ++lanCount;
//        isTree = isTree && g::allLans[lanNumber].isTreeRoot;
        g::allLans[lanNumber].partition = number;
        // Add one to the count of every host we touch. If two or more
        // LANs touch a host, then that host is sent to the children
        // as a hyper-edge. We just keep track of the count for
        // now. Later we'll worry about filtering them.
        list<string>::iterator pos = g::allLans[lanNumber].hosts.begin();
        list<string>::iterator limit = g::allLans[lanNumber].hosts.end();
        for (; pos != limit; ++pos)
        {
            ++(hostToOrder[*pos].count);
        }
    }
}

//-----------------------------------------------------------------------------

void Partition::dispatch(void)
{
    if (lanCount <= 1)
    {
        map<int, int>::iterator pos = lanToOrder.begin();
        map<int, int>::iterator limit = lanToOrder.end();
        for (; pos != limit; ++pos)
        {
            g::allLans[pos->first].assignment = pos->second;
        }
        return;
    }
    mapHosts();
    auto_ptr<FileT> pipe;
    ostringstream saver;
    string commandLine;
    if (isTree)
    {
        auto_assign(pipe, coprocess(g::treeCommand[0], &(g::treeCommand[0]),
                                    NULL));
        for (int i = 0; i < g::treeCommand.size(); ++i)
        {
            if (g::treeCommand[i] == NULL)
            {
                commandLine += "(null)";
            }
            else
            {
                commandLine += g::treeCommand[i];
                commandLine += ' ';
            }
        }
    }
    else
    {
        auto_assign(pipe, coprocess(g::generalCommand[0],
                                    &(g::generalCommand[0]), NULL));
        for (int i = 0; i < g::generalCommand.size(); ++i)
        {
            if (g::treeCommand[i] == NULL)
            {
                commandLine += "(null)";
            }
            else
            {
                commandLine += g::generalCommand[i];
                commandLine += ' ';
            }
        }
    }
    printGraph(saver);
    printGraph(pipe->input());
    pipe->closeIn();
    getNumbering(pipe->output());
    int childReturn = 0;
    int error = wait3(&childReturn, 0, NULL);
    if (error == -1)
    {
//        throw StringError(string("Waiting for child failed: ")
//                          + strerror(errno));
        cerr << "Waiting for child failed: " << strerror(errno) << endl;
        throw;
    }
    if (childReturn != 0)
    {
        cerr << "Error in partition " << number << ": ";
        parseError(pipe->error());
        cerr << endl << "Command line: " << commandLine << endl;
        cerr << "---Begin subfile---" << endl;
        cerr << saver.str();
        cerr << "---End subfile---" << endl;
        throw;
    }
}

//-----------------------------------------------------------------------------

int Partition::getLanCount(void)
{
    return lanCount;
}

//-----------------------------------------------------------------------------

void Partition::setTree(void)
{
    isTree = true;
}

//-----------------------------------------------------------------------------

void Partition::parseError(istream & error)
{
    string outside;
    string inside;
    bool done;
    getline(error, outside, '$');
    done = !error;
    while (!done)
    {
        cerr << outside;
        getline(error, inside, '$');
        bool hasInside = error;
        if (hasInside && inside == "")
        {
            cerr << "$";
        }
        else if (hasInside)
        {
            size_t index = inside.find('=');
            if (index != string::npos)
            {
                inside[index] = ' ';
            }
            istringstream buffer(inside);
            string name;
            int value = 0;
            buffer >> name >> value;
            if (!buffer)
            {
                buffer.clear();
                string newVal;
                buffer >> newVal;
                cerr << "[META ERROR: INVALID VALUE: name(" << name
                     << ") value(" << newVal << ")]";
            }
            else if (name == "vertex")
            {
                map<int, int>::iterator pos = orderToLan.find(value);
                if (pos == orderToLan.end())
                {
                    cerr << "[META ERROR: INVALID VERTEX NUMBER: name("
                         << name << ") value(" << value << ")]";
                }
                else
                {
                    cerr << pos->second;
                }
            }
            else if (name == "edge")
            {
                map<int, string>::iterator pos = orderToHost.find(value);
                if (pos == orderToHost.end())
                {
                    cerr << "[META ERROR: INVALID EDGE NUMBER: name("
                         << name << ") value(" << value << ")]";
                }
                else
                {
                    cerr << pos->second;
                }
            }
            else
            {
                cerr << "[META ERROR: INVALID NAME: name(" << name
                     << ") value(" << value << ")]";
            }
        }
        getline(error, outside, '$');
        done = !error;
    }
//    throw;
}

//-----------------------------------------------------------------------------

void Partition::printGraph(ostream & output)
{
    output << "vertex-count " << lanCount << endl;
    output << "edge-count " << hostCount << endl;
    output << "total-bits " << 32 << endl;
    output << "%%" << endl;
    map<int, int>::iterator lanPos = orderToLan.begin();
    map<int, int>::iterator lanLimit = orderToLan.end();
    for (; lanPos != lanLimit; ++lanPos)
    {
        output << "8 1 ";
        list<string>::iterator hostPos =
            g::allLans[lanPos->second].hosts.begin();
        list<string>::iterator hostLimit =
            g::allLans[lanPos->second].hosts.end();
        for (; hostPos != hostLimit; ++hostPos)
        {
            OrderCount oc = hostToOrder[*hostPos];
            if (oc.count > 1)
            {
                output << oc.order << " ";
            }
        }
        output << endl;
    }
}

//-----------------------------------------------------------------------------

void Partition::getNumbering(istream & input)
{
    string temp;
    input >> temp;
    input >> temp;
    input >> temp;
    input >> temp;
    int assignedNumber = 0;
    int order = 0;
    input >> assignedNumber;
    while (input)
    {
        g::allLans[orderToLan[order]].assignment = assignedNumber;
        input >> assignedNumber;
        ++order;
    }
}

void Partition::mapHosts(void)
{
    // For every host we've touched. If it has been touched by more
    // than two LANs, then set up mapping information.
    map<string, OrderCount>::iterator pos = hostToOrder.begin();
    map<string, OrderCount>::iterator limit = hostToOrder.end();
    for ( ; pos != limit; ++pos)
    {
        if (pos->second.count > 1)
        {
            pos->second.order = hostCount;
            orderToHost[hostCount] = pos->first;
            ++hostCount;
        }
    }
}

//-----------------------------------------------------------------------------
