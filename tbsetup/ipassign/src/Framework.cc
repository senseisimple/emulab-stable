// Framework.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Exception.h"
#include "Framework.h"
#include "Assigner.h"
#include "ConservativeAssigner.h"
#include "Router.h"
#include "HostRouter.h"
#include "LanRouter.h"
#include "NetRouter.h"

using namespace std;

Framework::Framework(int argCount, char ** argArray)
    : m_willTime(false)
    , m_usePartitionCount(false)
    , m_willPrintStatistics(false)
    , m_partitionCount(0)
{
    if (argCount <= 0 || argArray == NULL)
    {
        throw InvalidArgumentException("argCount or argArray are invalid");
    }
    parseCommandLine(argCount, argArray);
}

Framework::~Framework()
{
    // auto_ptr<>'s will automatically destroy dynamic memory
}

Framework::Framework(Framework const & right)
{
    // We don't know the types of the pointers in right. Therefore
    // we let them give us a copy of the proper type.
    if (right.m_assign.get() != NULL)
    {
        // This just means m_assign = right.m_assign->clone()
        // The left hand side becomes a clone of the right
        // the reason that we bend our backs here is because
        // of an obscure C++ rule involving rvalues.
        // You don't want to know.
        m_assign.reset(right.m_assign->clone().release());
    }
    if (right.m_route.get() != NULL)
    {
        // See above.
        m_route.reset(right.m_route->clone().release());
    }
}

Framework & Framework::operator=(Framework const & right)
{
    // This is a cool exception-safe way to do copy. See
    // Herb Sutter's 'Exceptional C++' book.
    Framework temp(right);
    swap(m_assign, temp.m_assign);
    swap(m_route, temp.m_route);
    return *this;
}

void Framework::input(istream & inputStream)
{
    string bufferString;
    size_t lanCount = 0;
    getline(inputStream, bufferString);
    while (inputStream)
    {
        stringstream buffer;
        buffer << bufferString;
        // make sure that this isn't just an empty line
        buffer >> ws;
        if (buffer.str() != "")
        {
            // check to see if there are any invalid characters
            for (size_t i = 0; i < bufferString.size(); ++i)
            {
                char temp = bufferString[i];
                if (!isdigit(temp) && temp != ' ' && temp != '\t'
                    && temp != 0x0A && temp != 0x0D)
                {
                    throw InvalidCharacterException(bufferString);
                }
            }
            // get the data from the line
            int bits = 0;
            int weight = 0;
            size_t temp;
            vector<size_t> nodes;
            buffer >> bits;
            if (!buffer)
            {
                // if the line contains characters other than whitespace and
                // we know that it only contains numbers and whitespace
                // therefore after reading in one number, buffer should not
                // have flagged an error.
                throw ImpossibleConditionException("Framework::input");
            }
            buffer >> weight;
            if (!buffer)
            {
                throw MissingWeightException(bufferString);
            }
            buffer >> temp;
            while (buffer)
            {
                nodes.push_back(temp);
                buffer >> temp;
            }
            if (nodes.size() < 2)
            {
                throw NotEnoughNodesException(bufferString);
            }
            // send it off to the
            ++lanCount;
            m_assign->addLan(bits, weight, nodes);
        }
        getline(inputStream, bufferString);
    }
    // If m_usePartitionCount is set, that is the number of partitions
    // to use. Otherwise, use the square root of the number of LANs.
    if (m_usePartitionCount)
    {
        m_assign->setPartitionCount(m_partitionCount);
    }
    else
    {
        m_assign->setPartitionCount(sqrt(lanCount));
    }
}

void Framework::ipAssign(void)
{
    m_assign->ipAssign();
}

void Framework::route(void)
{
    m_route->reset(*m_assign);
    m_route->calculateRoutes();
}

void Framework::printIP(ostream & output) const
{
    m_assign->print(output);
}

void Framework::printRoute(ostream & output) const
{
    m_route->print(output);
}

void Framework::printStatistics(ostream & output) const
{
    if (m_willPrintStatistics)
    {
        m_route->printStatistics(output);
    }
}

void Framework::parseCommandLine(int argCount, char ** argArray)
{
    AssignType assignChoice = SquareRoot;
    RouteType routeChoice = HostNet;
    bool isConservative = true;
    for (int i = 1; i < argCount; ++i)
    {
        string currentArg = argArray[i];
        if (currentArg.size() == 0 || currentArg[0] != '-')
        {
            throw InvalidArgumentException(currentArg);
        }
        else
        {
            parseArgument(currentArg, assignChoice, routeChoice,
                          isConservative);
        }
    }
    switch (assignChoice)
    {
    case Partition:
    case SquareRoot:
        if (isConservative)
        {
            m_assign.reset(new ConservativeAssigner);
        }
/*        else
        {
            m_assign.reset(new DynamicAssigner);
            }*/
        break;
/*    case Binary:
        m_assign.reset(new BinaryAssigner);
        break;
    case Greedy:
    m_assign.reset(new GreedyAssigner);
        break;*/
    default:
        throw ImpossibleConditionException("Framework::ParseCommandLine "
                                           "Variable: assignChoice");
        break;
    }
    switch (routeChoice)
    {
    case HostHost:
        m_route.reset(new HostRouter);
        break;
    case HostLan:
        m_route.reset(new LanRouter);
        break;
    case HostNet:
        m_route.reset(new NetRouter);
        break;
    default:
        throw ImpossibleConditionException("Framework::ParseCommandLine "
                                           "Variable: routeChoice");
        break;
    }
}

void Framework::parseArgument(string const & arg,
                              Framework::AssignType & assignChoice,
                              Framework::RouteType & routeChoice,
                              bool & isConservative)
{
    bool done = false;
    for (size_t j = 1; j < arg.size() && !done; ++j)
    {
        switch(arg[j])
        {
            // p means explicitly tell us the partitions
        case 'p':
            // if there is a 'p', it must go in its own -group
            // for example -p11 is legal but -p11h is not legal
            if (j == 1 && arg.size() > 1)
            {
                stringstream buffer;
                buffer << arg.substr(2);
                buffer >> m_partitionCount;
                assignChoice = Partition;
                m_usePartitionCount = true;
                done = true;
                if (buffer.fail())
                {
                    throw InvalidArgumentException(arg);
                }
            }
            else
            {
                throw InvalidArgumentException(arg);
            }
            break;
            // use the square root of the number of LANs for partitioning
        case 's':
            assignChoice = SquareRoot;
            m_usePartitionCount = false;
            break;
            // use a binary partitioning scheme
        case 'b':
            assignChoice = Binary;
            m_usePartitionCount = false;
            break;
            // use a greedy partitioning algorithm
        case 'g':
            assignChoice = Greedy;
            m_usePartitionCount = false;
            break;
            // use host-host routing
        case 'h':
            routeChoice = HostHost;
            break;
            // use host-lan routing
        case 'l':
            routeChoice = HostLan;
            break;
            // use host-net routing
        case 'n':
            routeChoice = HostNet;
            break;
            // time ip assignment and routing
        case 't':
            m_willTime = true;
            break;
            // print statistics about how well routing worked.
        case 'v':
            m_willPrintStatistics = true;
            break;
        case 'c':
            isConservative = true;
            break;
//      case 'r':
//          break;
        default:
            throw InvalidArgumentException(arg);
            break;
        }
    }
}




