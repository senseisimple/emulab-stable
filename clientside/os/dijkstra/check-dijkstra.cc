// check-dijkstra.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This file is based largely on testbed/tbsetup/ipassign/src/routestat.cc

// TODO: Modularize interface. Integrate backend with routestat.

// Checks that a set of routes is valid and provides statistics on how
// good the routes are.

// For information about command line arguments, see README

// If the argument '--logfile=<filename>' then the lengths from each
// node to each interface will be sent to <filename> in turn. The
// order output is sorted first by node number and second by the order
// the interface as listed in the .route file.

// If the argument '--histogram-data=<filenam>' then the number of
// routes for each node will be sent to <filename> in turn.

#include <string>
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <sstream>
#include <stack>
#include <iostream>

using namespace std;

#include "Exception.h"
#include "bitmath.h"

typedef long long int64;

class NoRouteException : public StringException
{
public:
    explicit NoRouteException(string const & error)
        : StringException("No route exists: " + error)
    {
    }
    virtual ~NoRouteException() throw() {}
};

class InvalidIPOnPathException : public StringException
{
public:
    explicit InvalidIPOnPathException(size_t current, IPAddress destination,
                                      IPAddress nextIP)
        : StringException("Invalid First Hop IP in route: ")
    {
        ostringstream buffer;
        buffer << "Node: " << current << " Destination: "
               << ipToString(destination) << " Offending IP: "
               << ipToString(nextIP);
        addToMessage(buffer.str());
    }
    virtual ~InvalidIPOnPathException() throw() {}
};

class InvalidNodeOnPathException : public StringException
{
public:
    explicit InvalidNodeOnPathException(size_t current, IPAddress destination,
                                        size_t next, IPAddress nextIP)
        : StringException("Invalid First Hop Node in route: ")
    {
        ostringstream buffer;
        buffer << "SourceNode: " << current << " Destination: "
               << ipToString(destination) << " FirstHop IP: "
               << ipToString(nextIP) << " Offending Node: " << next;
    }
    virtual ~InvalidNodeOnPathException() throw() {}
};

class ImpossibleRouteException : public StringException
{
public:
    explicit ImpossibleRouteException(size_t sourceNode, IPAddress destIP,
                                      IPAddress firstHop)
        : StringException("Impossible Route: ")
    {
        ostringstream buffer;
        buffer << "Node: " << sourceNode << " Destination: "
               << ipToString(destIP) << " FirstHop: " << ipToString(firstHop);
        addToMessage(buffer.str());
    }
    virtual ~ImpossibleRouteException() throw() {}
};

class CircularRouteException : public exception
{
public:
    explicit CircularRouteException(size_t newNext, IPAddress destination)
    {
        next = newNext;
        message = "Circular Route: Destination IP: " + ipToString(destination)
            + " Path: ";
    }

    virtual ~CircularRouteException() throw() {}

    void setPath(stack<size_t> & newPath)
    {
        ostringstream buffer;
        buffer << next;
        while (!(newPath.empty()))
        {
            buffer << " <- ";
            buffer << newPath.top();
            newPath.pop();
        }
        message += buffer.str();
    }

    virtual char const * what() const throw()
    {
        return message.c_str();
    }
private:
    string message;
    size_t next;
};

class FailedFileOpenException : public StringException
{
public:
    explicit FailedFileOpenException(string const & file)
        : StringException("Could not open file: " + file)
    {
    }
    virtual ~FailedFileOpenException() throw() {}
};

class LogException : public StringException
{
public:
    explicit LogException(std::exception const & other)
        : StringException(string("Log Error: ") + other.what())
    {
    }
    virtual ~LogException() throw() {}
};

class HistException : public StringException
{
public:
    explicit HistException(std::exception const & other)
        : StringException(string("Hist Error: ") + other.what())
    {
    }
    virtual ~HistException() throw() {}
};

class NoRouteFileException : public StringException
{
public:
    NoRouteFileException()
        : StringException("No route file specified. You need to add a 'routefile=<filename>' argument. See README for details.")
    {
    }
    virtual ~NoRouteFileException() throw() {}
};

class NoIpFileException : public StringException
{
public:
    NoIpFileException()
        : StringException("No ip file specified. You need to add an 'ipfile=<filename>' argument. See README for details.")
    {
    }
    virtual ~NoIpFileException() throw() {}
};

int maskToMaskSize(IPAddress mask)
{
    int size = 0;
    if ((mask & 0xffff0000) == 0xffff0000)
    {
        size += 16;
        mask <<= 16;
    }
    if ((mask & 0xff000000) == 0xff000000)
    {
        size += 8;
        mask <<= 8;
    }
    if ((mask & 0xf0000000) == 0xf0000000)
    {
        size += 4;
        mask <<= 4;
    }
    if ((mask & 0xc0000000) == 0xc0000000)
    {
        size += 2;
        mask <<= 2;
    }
    if ((mask & 0x80000000) == 0x80000000)
    {
        size += 1;
        mask <<= 1;
    }
    if (mask != 0)
    {
        size += 1;
    }
    return size;
}

class Route
{
public:
    Route(IPAddress newDestIP, size_t maskSize, IPAddress newFirstHop,
          IPAddress newSourceIP)
    {
        reset(newDestIP, maskSize, newFirstHop, newSourceIP);
    }
    explicit Route(string const & line)
    {
        reset(line);
    }
    bool operator==(IPAddress const & right) const
    {
        return (destIP & netMask) == (right & netMask);
    }
    // TODO: Is this really correct?
    bool operator<(Route const & right) const
    {
        return netMask > right.netMask
            || (netMask == right.netMask && destIP < right.destIP);
    }
    IPAddress getFirstHop(void) const
    {
        return firstHop;
    }
    IPAddress getSource(void) const
    {
        return sourceIP;
    }
    void reset(IPAddress newDestIP, size_t maskSize, IPAddress newFirstHop,
               IPAddress newSourceIP)
    {
        destIP = newDestIP;
        netMask = 0xffffffff << (32 - maskSize);
        if (maskSize == 0)
        {
            netMask = 0;
        }
        firstHop = newFirstHop;
        sourceIP = newSourceIP;
    }
    void reset(Route const & right)
    {
        netMask = right.netMask;
        destIP = right.destIP;
        firstHop = right.firstHop;
        sourceIP = right.sourceIP;
    }
    void reset(string const & line)
    {
        size_t destIndex = line.find("=") + 1;
        size_t destSize = line.find(" ", destIndex) - destIndex;

        size_t maskIndex = line.find("=", destIndex) + 1;
        maskIndex = line.find("=", maskIndex) + 1;
        size_t maskSize = line.find(" ", maskIndex) - maskIndex;

        size_t hopIndex = line.find("=", maskIndex) + 1;
        size_t hopSize = line.find(" ", hopIndex) - hopIndex;

        size_t sourceIndex = line.find("=", hopIndex) + 1;
        sourceIndex = line.find("=", sourceIndex) + 1;
        size_t sourceSize = line.find(" ", sourceIndex) - sourceIndex;

        reset(stringToIP(line.substr(destIndex, destSize)),
              maskToMaskSize(stringToIP(line.substr(maskIndex, maskSize))),
              stringToIP(line.substr(hopIndex, hopSize)),
              stringToIP(line.substr(sourceIndex, sourceSize)));
/*
        // TODO: Make this less brittle
        size_t destIndex = line.find(":") + 2;
        size_t hopIndex = line.find(":", destIndex) + 2;
        size_t maskIndex = line.find("/") + 1;
        istringstream maskBuffer(line.substr(maskIndex, 2));
        size_t maskSize = 0;
        maskBuffer >> maskSize;
        reset(stringToIP(line.substr(destIndex, maskIndex - destIndex)),
              maskSize, stringToIP(line.substr(hopIndex)));
*/
    }
    void debugPrint(ostream & output) const
    {
        output << ipToString(destIP) << "/" << hex << netMask << dec
               << "->" << ipToString(firstHop);
    }
private:
    IPAddress netMask;
    IPAddress destIP;
    IPAddress firstHop;
    IPAddress sourceIP;
};

bool operator==(IPAddress const & left, Route const & right)
{
    return right == left;
}

class Node
{
public:
    Node() : visited(false), valid(false) {}
    void addRoute(Route const & newRoute) { routes.insert(newRoute); }
    void addNeighbor(IPAddress neighbor)
    {
        neighborIPs.insert(neighbor);
    }
    void setVisited(void) { visited = true; }
    void clearVisited(void) { visited = false; }
    void setValid(void) { valid = true; }
    void clearValid(void) { valid = false; }

    bool isNeighbor(IPAddress neighbor) const
    {
        return neighborIPs.count(neighbor) > 0;
    }
    bool wasVisited(void) const { return visited; }
    bool isValid(void) const { return valid; }
    IPAddress routeTo(IPAddress destination) const
    {
        IPAddress result = 0;
        set<Route>::const_iterator pos = routes.begin();
        bool done = false;
        for ( ; pos != routes.end() && !done; ++pos)
        {
            if (*pos == destination)
            {
                result = pos->getFirstHop();
                done = true;
            }
        }
        if (result == 0)
        {
            throw NoRouteException("ToIP: " + ipToString(destination));
        }
        return result;
    }
    void debugPrint(ostream & output) const
    {
        {
            set<Route>::const_iterator pos = routes.begin();
            for ( ; pos != routes.end(); ++pos)
            {
                output << "Route: ";
                pos->debugPrint(output);
                output << endl;
            }
        }
        {
            set<IPAddress>::const_iterator pos = neighborIPs.begin();
            for ( ; pos != neighborIPs.end(); ++pos)
            {
                output << "Neighbor: " << ipToString(*pos) << endl;
            }
        }
        if (visited)
        {
            output << "visited ";
        }
        if (valid)
        {
            output << "valid ";
        }
        output << endl << "*****" << endl;
    }
private:
    set<Route> routes;
    // IPAddress of neighbor's interface
    set<IPAddress> neighborIPs;
    bool visited;
    bool valid;
};

namespace arg
{
    bool shouldLog = false;
    ofstream log;
    bool shouldHist = false;
    ofstream hist;
    ifstream ip;
    ifstream route;
}

void parseArgs(int argc, char * argv[]);

// take the ip assignment portion of the output and use that to set up
// ip addresses and adjascency for the nodes
void setupNodes(vector<Node> & nodes, map<IPAddress,size_t> & interfaces,
                list<IPAddress> & interfaceOrder);

// For each node's section, set up the routing table for that node.
// Return the total number of routes found.
int64 setupRoutes(vector<Node> & nodes);

// Check each route for cycles and ensure that each route is possible.
// Return the aggregate length of routes from every node to every
//   interface.
int64 tryRoutes(vector<Node> & nodes,
                map<IPAddress,size_t> const & interfaces,
                list<IPAddress> const & interfaceOrder);

// Check a particular route for cycles and ensure that it is possible.
// Return the length of that route.
int64 followRoute(size_t source, IPAddress destination, size_t destNode,
                  vector<Node> & nodes,
                  map<IPAddress,size_t> const & interfaces);

// Ensures that the next hop is kosher. If something is wrong, throws
// an exception.
void checkHop(IPAddress destination, size_t current,
              map<IPAddress,size_t>::const_iterator found, IPAddress nextIP,
              vector<Node> const & nodes,
              map<IPAddress,size_t> const & interfaces);

// Format and print the total number of routes and the aggregate length.
void displayResults(size_t routeCount, size_t routeLength);

// Clear the visited flag for every node. The visited flag is used to detect
// cycles in a route.
void clearVisited(vector<Node> & nodes);

int main(int argc, char * argv[])
{
    int result = 0;
    try
    {
        parseArgs(argc, argv);
        vector<Node> nodes;
        map<IPAddress,size_t> interfaces;
        list<IPAddress> interfaceOrder;
        setupNodes(nodes, interfaces, interfaceOrder);
        int64 routeCount = setupRoutes(nodes);
        int64 routeLength = tryRoutes(nodes, interfaces, interfaceOrder);
        displayResults(routeCount, routeLength);
    }
    catch(exception & error)
    {
        cerr << "check: " << error.what() << endl;
        result = 1;
    }
    return result;
}

void parseArgs(int argc, char * argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--logfile=", sizeof("--logfile=") - 1) == 0)
        {
            char * fileName = argv[i] + sizeof("--logfile=") - 1;
            arg::shouldLog = true;
            arg::log.open(fileName, ios::out | ios::trunc);
            if (!(arg::log))
            {
                throw LogException(FailedFileOpenException(fileName));
            }
        }
        else if (strncmp(argv[i], "--histogram-data=",
                 sizeof("--histogram-data=") - 1) == 0)
        {
            char * fileName = argv[i] + sizeof("--histogram-data=") - 1;
            arg::shouldHist = true;
            arg::hist.open(fileName, ios::out | ios::trunc);
            if (!(arg::hist))
            {
                throw HistException(FailedFileOpenException(fileName));
            }
        }
        else if (strncmp(argv[i], "--ipfile=", sizeof("--ipfile=") - 1) == 0)
        {
            char * fileName = argv[i] + sizeof("--ipfile=") - 1;
            arg::ip.open(fileName, ios::in);
            if (!(arg::ip))
            {
                throw FailedFileOpenException(fileName);
            }
        }
        else if (strncmp(argv[i], "--routefile=",
                         sizeof("--routefile=") - 1) == 0)
        {
            char * fileName = argv[i] + sizeof("--routefile=") - 1;
            arg::route.open(fileName, ios::in);
            if (!(arg::route))
            {
                throw FailedFileOpenException(fileName);
            }
        }
        else
        {
            throw InvalidArgumentException(argv[i]);
        }
    }
    if (!(arg::ip))
    {
        throw NoIpFileException();
    }
    if (!(arg::route))
    {
        throw NoRouteFileException();
    }
}

void setupNodes(vector<Node> & nodes, map<IPAddress,size_t> & interfaces,
                list<IPAddress> & interfaceOrder)
{
    string bufferString;
    size_t lastLan = 0;
    list< pair<size_t,IPAddress> > lastNodes;

    interfaces.clear();
    interfaceOrder.clear();

    getline(arg::ip, bufferString);
    while (arg::ip)
    {
        istringstream buffer(bufferString);
        size_t currentLan = 0;
        size_t currentNode = 0;
        string ipString;
        buffer >> currentLan >> currentNode >> ipString;
        IPAddress ip = stringToIP(ipString);
        if (buffer)
        {
            if (currentNode >= nodes.size())
            {
                nodes.resize(currentNode + 1);
            }
            interfaces[ip] = currentNode;
            interfaceOrder.push_back(ip);
            nodes[currentNode].setValid();
            if (currentLan == lastLan)
            {
                list< pair<size_t,IPAddress> >::iterator pos;
                for (pos = lastNodes.begin(); pos != lastNodes.end(); ++pos)
                {
                    nodes[pos->first].addNeighbor(ip);
                    nodes[currentNode].addNeighbor(pos->second);
                }
                lastNodes.push_back(make_pair(currentNode, ip));
            }
            else
            {
                lastLan = currentLan;
                lastNodes.clear();
                lastNodes.push_back(make_pair(currentNode, ip));
            }
        }

        getline(arg::ip, bufferString);
    }
}

int64 setupRoutes(vector<Node> & nodes)
{
    int64 result = 0;
    string bufferString;

    getline(arg::route, bufferString);
    while (arg::route)
    {
        istringstream buffer(bufferString);
        size_t nodeNumber = 0;
        buffer >> nodeNumber;
        if (buffer && nodeNumber < nodes.size())
        {
            int64 routesOnThisNode = 0;
            getline(arg::route, bufferString);
            while (arg::route && bufferString != "%%")
            {
                nodes[nodeNumber].addRoute(Route(bufferString));
                ++routesOnThisNode;
                getline(arg::route, bufferString);
            }
            result += routesOnThisNode;
            if (arg::shouldHist)
            {
                arg::hist << routesOnThisNode << endl;
            }
        }
        getline(arg::route, bufferString);
    }

    return result;
}

int64 tryRoutes(vector<Node> & nodes,
                map<IPAddress,size_t> const & interfaces,
                list<IPAddress> const & interfaceOrder)
{
    int64 result = 0;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].isValid())
        {
            list<IPAddress>::const_iterator pos = interfaceOrder.begin();
//        map<IPAddress,size_t>::const_iterator pos = interfaces.begin();
            for ( ; pos != interfaceOrder.end(); ++pos)
            {
                map<IPAddress, size_t>::const_iterator found;
                found = interfaces.find(*pos);
                if (found == interfaces.end())
                {
                    throw StringException("Impossible condition: interfaces");
                }
                //                   (source, destIP, destNode,
                result += followRoute(i,      *pos,   found->second,
                                      nodes, interfaces);


            }
            if (i % 10 == 0)
            {
                cerr << i;
            }
            else
            {
                cerr << '.';
            }
        }
    }
    return result;
}

int64 followRoute(size_t source, IPAddress destination, size_t destNode,
                   vector<Node> & nodes,
                   map<IPAddress,size_t> const & interfaces)
{
    int64 result = 0;
    size_t current = source;

    stack<size_t> path;
    path.push(current);

    clearVisited(nodes);
    nodes[source].setVisited();
    try
    {
        while (current != destNode)
        {
            IPAddress nextIP = 0;
            map<IPAddress,size_t>::const_iterator found;
            if (nodes[current].isNeighbor(destination))
            {
                nextIP = destination;
            }
            else
            {
                nextIP = nodes[current].routeTo(destination);
            }
            found = interfaces.find(nextIP);
            checkHop(destination, current, found, nextIP, nodes, interfaces);
            // The next hop looks good.
            nodes[found->second].setVisited();
            current = found->second;
            ++result;
            path.push(current);
        }
    }
    catch (NoRouteException & error)
    {
        ostringstream currentBuffer;
        currentBuffer << " FromNode: " << current;
        error.addToMessage(currentBuffer.str());
        throw error;
    }
    catch (CircularRouteException & error)
    {
        error.setPath(path);
        throw error;
    }

    if (arg::shouldLog)
    {
        arg::log << result << endl;
    }
    return result;
}

void checkHop(IPAddress destination, size_t current,
              map<IPAddress,size_t>::const_iterator found, IPAddress nextIP,
              vector<Node> const & nodes,
              map<IPAddress,size_t> const & interfaces)
{
    if (found == interfaces.end())
    {
        // Error. The next IP address is invalid
        throw InvalidIPOnPathException(current, destination, nextIP);
    }
    size_t next = found->second;
    if (next > (nodes.size() - 1) || !(nodes[found->second].isValid()))
    {
        // Error. The next node is invalid
        throw InvalidNodeOnPathException(current, destination, next, nextIP);
    }
    else if (!(nodes[current].isNeighbor(nextIP)))
    {
        // Error. The next hop isn't adjascent to the current node.
        throw ImpossibleRouteException(current, destination, nextIP);
    }
    else if (nodes[next].wasVisited())
    {
        // Error. The next hop has already been visited
        // (circular route).
        throw CircularRouteException(next, destination);
    }
}

void displayResults(size_t routeCount, size_t routeLength)
{
    cout << "RouteCount: " << routeCount << endl;
    cout << "AggregateRouteLength: " << routeLength << endl;
}

void clearVisited(vector<Node> & nodes)
{
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        nodes[i].clearVisited();
    }
}
