// routestat.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Checks that a set of routes is valid and provides statistics on how
// good the routes are.

// If the arguments are '-l <filename>' then the lengths from each node
// to each interface will be sent to <filename> in turn. The order of the
// output is first by node number and second by the order the interface is
// listed in the .route file.

#include "lib.h"
#include "Exception.h"
#include <fstream>

using namespace std;

typedef long long int64;

class NoRouteException : public StringException
{
public:
    explicit NoRouteException(string const & error)
        : StringException("No route exists: " + error)
    {
    }
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
};

class CircularRouteException : public StringException
{
public:
    explicit CircularRouteException(size_t current, IPAddress destination)
        : StringException("Circular Route: ")
    {
        ostringstream buffer;
        buffer << "Node: " << current << " Destination: "
               << ipToString(destination);
    }
};

class FailedFileOpenException : public StringException
{
public:
    explicit FailedFileOpenException(string const & file)
        : StringException("Could not open file: " + file)
    {
    }
};

class LogException : public StringException
{
public:
    explicit LogException(std::exception const & other)
        : StringException(string("Log Error: ") + other.what())
    {
    }
};

class Route
{
public:
    Route(IPAddress newDestIP, size_t maskSize, IPAddress newFirstHop)
    {
        reset(newDestIP, netMask, newFirstHop);
    }
    explicit Route(string const & line)
    {
        reset(line);
    }
    bool operator==(IPAddress const & right) const
    {
        return (destIP & netMask) == (right & netMask);
    }
    bool operator<(Route const & right) const
    {
        return netMask < right.netMask
            || (netMask == right.netMask && destIP < right.destIP);
    }
    IPAddress getFirstHop(void) const
    {
        return firstHop;
    }
    void reset(IPAddress newDestIP, size_t maskSize, IPAddress newFirstHop)
    {
        destIP = newDestIP;
        netMask = 0xffffffff << (32 - maskSize);
        firstHop = newFirstHop;
    }
    void reset(Route const & right)
    {
        netMask = right.netMask;
        destIP = right.destIP;
        firstHop = right.firstHop;
    }
    void reset(string const & line)
    {
        // TODO: Make this less brittle
        size_t destIndex = line.find(":") + 2;
        size_t hopIndex = line.find(":", destIndex) + 2;
        size_t maskIndex = line.find("/") + 1;
        istringstream maskBuffer(line.substr(maskIndex, 2));
        size_t maskSize = 0;
        maskBuffer >> maskSize;
        reset(stringToIP(line.substr(destIndex, maskIndex - destIndex)),
              maskSize, stringToIP(line.substr(hopIndex)));
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
    // IPAdress of neighbor's interface
    set<IPAddress> neighborIPs;
    bool visited;
    bool valid;
};

namespace arg
{
    bool shouldLog = false;
    ofstream log;
}

void parseArgs(int argc, char * argv[]);

// take the ip assignment portion of the output and use that to set up
// ip addresses and adjascency for the nodes
void setupNodes(vector<Node> & nodes, map<IPAddress,size_t> & interfaces);

// For each node's section, set up the routing table for that node.
// Return the total number of routes found.
int64 setupRoutes(vector<Node> & nodes);

// Check each route for cycles and ensure that each route is possible.
// Return the aggregate length of routes from every node to every
//   interface.
int64 tryRoutes(vector<Node> & nodes,
                map<IPAddress,size_t> const & interfaces);

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
        setupNodes(nodes, interfaces);
        int64 routeCount = setupRoutes(nodes);
        int64 routeLength = tryRoutes(nodes, interfaces);
        displayResults(routeCount, routeLength);
    }
    catch(exception & error)
    {
        cerr << "routestat: " << error.what() << endl;
        result = 1;
    }
    return result;
}

void parseArgs(int argc, char * argv[])
{
    if (argc == 3)
    {
        if (argv[1] == string("-l") && argv[2] != string(""))
        {
            arg::shouldLog = true;
            arg::log.open(argv[2], ios::out | ios::trunc);
            if (!(arg::log))
            {
                throw LogException(FailedFileOpenException(argv[2]));
            }
        }
    }
}

void setupNodes(vector<Node> & nodes, map<IPAddress,size_t> & interfaces)
{
    string bufferString;
    size_t lastLan = 0;
    list< pair<size_t,IPAddress> > lastNodes;

    interfaces.clear();

    getline(cin, bufferString);
    while (cin && bufferString != "%%")
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

        getline(cin, bufferString);
    }
}

int64 setupRoutes(vector<Node> & nodes)
{
    int64 result = 0;
    string bufferString;

    getline(cin, bufferString);
    while (cin)
    {
        istringstream buffer(bufferString);
        string temp;
        buffer >> temp >> temp >> temp >> temp;
        size_t nodeNumber = 0;
        buffer >> nodeNumber;
        if (buffer && nodeNumber < nodes.size())
        {
            getline(cin, bufferString);
            while (cin && bufferString != "%%")
            {
                nodes[nodeNumber].addRoute(Route(bufferString));
                ++result;
                getline(cin, bufferString);
            }
        }
        getline(cin, bufferString);
    }

    return result;
}

int64 tryRoutes(vector<Node> & nodes,
                 map<IPAddress,size_t> const & interfaces)
{
    int64 result = 0;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        map<IPAddress,size_t>::const_iterator pos = interfaces.begin();
        for ( ; pos != interfaces.end(); ++pos)
        {
            if (nodes[i].isValid())
            {
                //                   (source, dest, nodes, interfaces)
                result += followRoute(i, pos->first, pos->second,
                                      nodes, interfaces);
            }
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
    return result;
}

int64 followRoute(size_t source, IPAddress destination, size_t destNode,
                   vector<Node> & nodes,
                   map<IPAddress,size_t> const & interfaces)
{
    int64 result = 0;
    size_t current = source;

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
        }
    }
    catch (NoRouteException & error)
    {
        ostringstream currentBuffer;
        currentBuffer << " FromNode: " << current;
        error.addToMessage(currentBuffer.str());
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
        throw CircularRouteException(current, destination);
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
