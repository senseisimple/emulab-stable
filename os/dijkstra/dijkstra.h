// dijkstra.h

#ifndef DIJKSTRA_H_DISTRIBUTED_DIJKSTRA_1
#define DIJKSTRA_H_DISTRIBUTED_DIJKSTRA_1

// Map every pair of adjascent hosts to the IP addresses of their interfaces.
typedef std::vector< std::multimap<int,std::pair<std::string,std::string> > >
                                                               HostHostToIpMap;

// A convenient converter
std::string intToString(int num);
void printRouteToIp(string sourceIp, string firstHopIp, string destIp,
                    int cost);
void printRouteToSubnet(string sourceIp, string firstHopIp, string destSubnet,
                        int netMaskSize, int cost);


// This exception gets thrown if <name> in the 'source=<name>'
// argument is invalid. This is true if <name> doesn't correspond to
// any label mentioned in the input.
class InvalidSourceException : public std::exception
{
public:
    InvalidSourceException(string sourceName)
    {
        message = "Invalid source argument: name=" + sourceName;
    }

    virtual char const * what(void) const
    {
        return message.c_str();
    }
private:
    string message;
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

    void setMessage(string newSource, string newDest, string newFirstHop)
    {
        message = "Route not found: source=" + newSource + " dest=" + newDest
            + " firstHop=" + newFirstHop;
    }

    virtual char const * what(void) const
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
    string message;
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

    virtual char const * what(void) const
    {
        return message.c_str();
    }
private:
    string message;
};

#endif
