// dijkstra.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef DIJKSTRA_H_DISTRIBUTED_DIJKSTRA_1
#define DIJKSTRA_H_DISTRIBUTED_DIJKSTRA_1

// Print a route from a host to a particular destination IP address.
void printRouteToIp(std::string sourceIp, std::string firstHopIp,
                    std::string destIp, int cost);

// Print a route from a host to a particular destination subnet.
void printRouteToSubnet(std::string sourceIp, std::string firstHopIp,
                        std::string destSubnet, int netMaskSize, int cost);

// This exception gets thrown if <name> in the 'source=<name>'
// argument is invalid. This is true if <name> doesn't correspond to
// any label mentioned in the input.
class InvalidSourceException : public std::exception
{
public:
    InvalidSourceException(std::string sourceName)
    {
        message = "Invalid source argument: source=" + sourceName;
    }

    virtual ~InvalidSourceException() throw() {}

    virtual char const * what(void) const throw()
    {
        return message.c_str();
    }
private:
    std::string message;
};

class RouteNotFoundException : public std::exception
{
public:
    RouteNotFoundException(int newSource, int newDest, int newFirstHop)
        : source(newSource)
        , dest(newDest)
        , firstHop(newFirstHop)
    {
        setMessage(intToString(source), intToString(dest),
                   intToString(firstHop));
    }

    virtual ~RouteNotFoundException() throw() {}

    void setMessage(std::string newSource, std::string newDest,
                    std::string newFirstHop)
    {
        message = "Route not found: source=" + newSource + " dest=" + newDest
            + " firstHop=" + newFirstHop;
    }

    virtual char const * what(void) const throw()
    {
        return message.c_str();
    }

    int getSource(void)
    {
        return source;
    }

    int getDest(void)
    {
        return dest;
    }

    int getFirstHop(void)
    {
        return firstHop;
    }
private:
    int source;
    int dest;
    int firstHop;
    std::string message;
};

class TooManyHostsException : public std::exception
{
public:
    TooManyHostsException(int maxHostCount)
    {
        message = "You said that there would only be "
            + intToString(maxHostCount) + " hosts. I just found host #"
            + intToString(maxHostCount + 1) + ".";
    }

    virtual ~TooManyHostsException() throw() {}

    virtual char const * what(void) const throw()
    {
        return message.c_str();
    }
private:
    std::string message;
};

#endif
