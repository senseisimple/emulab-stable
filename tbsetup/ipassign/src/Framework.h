// Framework.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Framework provides the infrastructure that selects between which
// algorithms to use for ip assignment, partitioning, and routing

#ifndef FRAMEWORK_H_IP_ASSIGN_2
#define FRAMEWORK_H_IP_ASSIGN_2

class Assigner;
class Router;
class Partition;

class Framework
{
public:
    // The constructor processes the command line arguments and sets up
    // the algorithm types based on them.
    Framework(int argCount = 0, char ** argArray = NULL);
    ~Framework();
    Framework(Framework const & right);
    Framework & operator=(Framework const & right);

    // Read the list of LANs from the given stream and send them to the
    // Assigner.
    void input(std::istream & inputStream);

    // Assign IP addresses.
    void ipAssign(void);

    // Assign routes.
    void route(void);

    // Print IP addresses.
    void printIP(ostream & output) const;

    // Print routes.
    void printRoute(ostream & output) const;
private:
    // These constants are used to store the module options before the
    // modules themselves are created according to the specification.
    enum AssignType
    {
        Conservative, Adaptive, Binary, Greedy
    };
    enum PartitionType
    {
        Fixed, SquareRoot, Search
    };
    enum RouteType
    {
        HostHost, HostLan, HostNet
    };
private:
    // The actual gruntwork of parsing the arguments
    void parseCommandLine(int argCount, char ** argArray);
    void parseArgument(std::string const & arg, AssignType & assignChoice,
                       RouteType & routeChoice);
private:
    // The selector for which IP assignment strategy to use
    std::auto_ptr<Assigner> m_assign;
    // The selector for which routing strategy to use
    std::auto_ptr<Router> m_route;
    // The selector for which partitioning strategy to use
    std::auto_ptr<Partition> m_partition;
};

#endif








