// Framework.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Framework provides the infrastructure that selects between which
// algorithms to use for ip assignment and routing

#ifndef FRAMEWORK_H_IP_ASSIGN_2
#define FRAMEWORK_H_IP_ASSIGN_2

class Assigner;
class Router;
class Partition;

class Framework
{
public:
    Framework(int argCount = 0, char ** argArray = NULL);
    ~Framework();
    Framework(Framework const & right);
    Framework & operator=(Framework const & right);
    void input(std::istream & inputStream);
    void ipAssign(void);
    void route(void);
    void printIP(ostream & output) const;
    void printRoute(ostream & output) const;
private:
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
    void parseCommandLine(int argCount, char ** argArray);
    void parseArgument(std::string const & arg, AssignType & assignChoice,
                       RouteType & routeChoice);
private:
    std::auto_ptr<Assigner> m_assign;
    std::auto_ptr<Router> m_route;
    std::auto_ptr<Partition> m_partition;
};

#endif








