// prepass.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef PREPASS_H_IPASSIGN_3
#define PREPASS_H_IPASSIGN_3

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <cstdlib>

#include <signal.h>
#include <sys/wait.h>

struct Lan
{
    Lan() : number(0),
            partition(-1),
            isNamingPoint(false),
            isTreeRoot(false),
            parentIndex(0),
            assignment(0),
            time (0) {}
    // This is the index of the LAN in the great LAN array.
    int number;
    // The partition index number of the LAN.
    int partition;
    // A list of host keys used by the LAN.
    std::list<std::string> hosts;
    // Whether this LAN names a partition or not.
    bool isNamingPoint;
    // Whether this LAN is part of a tree or a cycle.
    bool isTreeRoot;
    // The index of the parent of this LAN in the DFS tree.
    int parentIndex;
    // The address assigned to this LAN. Used as part of the IP address.
    int assignment;
    // The time that this LAN was visited in the DFS tree.
    int time;
    // This is the hostname that is the edge currently being explored
    // in the DFS tree. We must keep track of this to prevent the same
    // edge being used more than once in a cycle.
    //
    // After finishing the sub-tree rooted at a host, we can ignore
    // this because no other host can touch it to check the
    // forwardEdge. Likewise, we need only concern ourselves with the
    // current host we are exploring. Previously explored hosts will
    // never be touched again.
    std::string forwardEdge;
};

struct Host
{
    Host() {}
    std::string name;
    std::list<int> lans;
};

class Partition;

namespace g
{
    extern std::map<std::string, Host> allHosts;
    extern std::vector<Lan> allLans;
    extern std::vector<Partition> allPartitions;
    extern std::vector<char*> treeCommand;
    extern std::vector<char*> generalCommand;

    extern bool useNeato;
}

void init(void);
void parseArgs(int argc, char * argv[]);
void parseSingleArg(char * arg);
void parseInput(void);
void addLan(int bits, int weights, vector<string> const & nodes);

void breakGraph(void);

void findNumbers(void);
void ouputAddresses(void);
void normalOutput(void);
void neatoOutput(void);

#endif
