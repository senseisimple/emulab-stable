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
    void printStatistics(ostream & output) const;
private:
    enum AssignType
    {
        Partition, SquareRoot, Binary, Greedy
    };
    enum RouteType
    {
        HostHost, HostLan, HostNet
    };
private:
    void parseCommandLine(int argCount, char ** argArray);
    void parseArgument(std::string const & arg, AssignType & assignChoice,
                       RouteType & routeChoice, bool & isConservative);
private:
    std::auto_ptr<Assigner> m_assign;
    std::auto_ptr<Router> m_route;
    bool m_willTime;
    bool m_usePartitionCount;
    bool m_willPrintStatistics;
    int m_partitionCount;
};

#endif
