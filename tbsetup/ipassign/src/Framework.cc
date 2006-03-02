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
#include "HierarchicalAssigner.h"
#include "Partition.h"
#include "FixedPartition.h"
#include "SearchPartition.h"
#include "SquareRootPartition.h"
#include "RatioCutPartition.h"
#include "CutSearchPartition.h"

using namespace std;

Framework::Framework(int argCount, char ** argArray)
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
        // The left hand side becomes a clone of the right.
        // The reason that we bend our backs here is because
        // of an obscure C++ rule involving rvalues.
        // You don't want to know.
        m_assign.reset(right.m_assign->clone().release());
    }
    if (right.m_partition.get() != NULL)
    {
        // See above.
        m_partition.reset(right.m_partition->clone().release());
    }
}

Framework & Framework::operator=(Framework const & right)
{
    // This is a cool exception-safe way to do copy. See
    // Herb Sutter's 'Exceptional C++' book.
    Framework temp(right);
    swap_auto_ptr(m_assign, temp.m_assign);
    swap_auto_ptr(m_partition, temp.m_partition);
    return *this;
}

void Framework::input(istream & inputStream)
{
    string bufferString;
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
                // the line contains characters other than whitespace and
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
            m_partition->addLan();
            m_assign->addLan(bits, weight, nodes);
        }
        getline(inputStream, bufferString);
    }
}

// These functions simply delegate the commands.
void Framework::ipAssign(void)
{
    m_assign->ipAssign();
}

void Framework::printIP(ostream & output) const
{
    m_assign->print(output);
}

void Framework::parseCommandLine(int argCount, char ** argArray)
{
    AssignType assignChoice = Hierarchical;
    m_partition.reset(new SquareRootPartition());
    for (int i = 1; i < argCount; ++i)
    {
        string currentArg = argArray[i];
        if (currentArg.size() == 0 || currentArg[0] != '-')
        {
            throw InvalidArgumentException(currentArg);
        }
        else
        {
            parseArgument(currentArg, assignChoice);
        }
    }
    switch (assignChoice)
    {
    case Conservative:
        m_assign.reset(new ConservativeAssigner(*m_partition));
        break;
    case Hierarchical:
        m_assign.reset(new HierarchicalAssigner(*m_partition));
        break;
/*    case Binary:
        m_assign.reset(new BinaryAssigner(*m_partition));
        break;
    case Greedy:
    m_assign.reset(new GreedyAssigner(*m_partition));
        break;*/
    default:
        throw ImpossibleConditionException("Framework::ParseCommandLine "
                                           "Variable: assignChoice");
        break;
    }
}

void Framework::parseArgument(string const & arg,
                              Framework::AssignType & assignChoice)
{
    bool done = false;
    for (size_t j = 1; j < arg.size() && !done; ++j)
    {
        switch(arg[j])
        {
            // p means this argument group is about partitions. It must be
            // followed by a number (Fixed), a 'q' (SquareRoot),
            // an 's' (Search), or an 'r' (RatioCut)
        case 'p':
            // if there is a 'p', it must go in its own -group
            // for example -p11 is legal but -p11h is not legal
            if (j == 1 && arg.size() > 1)
            {
                if (isdigit(arg[2]))
                {
                    int partitionCount = 0;
                    stringstream buffer;
                    buffer << arg.substr(2);
                    buffer >> partitionCount;
                    if (buffer.fail())
                    {
                        throw InvalidArgumentException(arg);
                    }
                    m_partition.reset(new FixedPartition(partitionCount));
                    done = true;
                }
                else if (arg[2] == 'q')
                {
                    m_partition.reset(new SquareRootPartition);
                    done = true;
                }
                else if (arg[2] == 's')
                {
                    m_partition.reset(new SearchPartition);
                    done = true;
                }
                else if (arg[2] == 'r')
                {
                    m_partition.reset(new RatioCutPartition);
                    done = true;
                }
                else if (arg[2] == 'c')
                {
                    m_partition.reset(new CutSearchPartition);
                    done = true;
                }
                else
                {
                    throw InvalidArgumentException(arg);
                }
            }
            else
            {
                throw InvalidArgumentException(arg);
            }
            break;
        case 'c':
            // use conservative ip assignment
            assignChoice = Conservative;
            break;
            // use a binary partitioning scheme
        case 'b':
            assignChoice = Binary;
            break;
            // use a greedy partitioning algorithm
        case 'g':
            assignChoice = Greedy;
            break;
        case '!':
            break;
        default:
            throw InvalidArgumentException(arg);
            break;
        }
    }
}
