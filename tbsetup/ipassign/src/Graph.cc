// Graph.cc

#include "lib.h"
#include "Exception.h"
#include "Graph.h"

// if this is defined, output is the raw numbers chosen rather than
// the ip address.
// #define IP_ASSIGN_DEBUG

using namespace std;

//////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////

Graph::Graph(istream & input)
    : m_largestLan(0)
{
    reset(input);
}

Graph::~Graph()
{
}

//////////////////////////////////////////////////////
// copy
//////////////////////////////////////////////////////

//Graph::Graph(Graph const & right)
//{
//}

//Graph::Graph & operator=(Graph const & right)
//{
//    return *this;
//}

//////////////////////////////////////////////////////
// statistics
//////////////////////////////////////////////////////

int Graph::getLargestLan(void) const
{
    return m_largestLan;
}

int Graph::getLanCount(void) const
{
    return m_lanList.size();
}

//////////////////////////////////////////////////////
// information
//////////////////////////////////////////////////////

void Graph::convert(std::vector<int> & indexes, std::vector<int> & neighbors,
             std::vector<int> & weights) const
{
    indexes.clear();
    neighbors.clear();
    weights.clear();
    indexes.reserve(m_nodes.size() + 1);

    list<Lan>::const_iterator lanPos = m_lanList.begin();
    for ( ; lanPos != m_lanList.end(); ++lanPos)
    {
        indexes.push_back(neighbors.size());
        convertAddLan(*lanPos, neighbors, weights);
    }
    indexes.push_back(neighbors.size());
}

//////////////////////////////////////////////////////
// change
//////////////////////////////////////////////////////

void Graph::reset(istream & input)
{
    // remove old state
    m_largestLan = 0;
    m_lanList.clear();
    m_nodes.clear();

    // read in the input
    string line;
    getline(input, line);
    while (input)
    {
        try
        {
            Lan temp;
            temp.number = m_lanList.size();
            temp.weight = parseLine(line, temp.nodes);
            m_largestLan = max(static_cast<size_t>(m_largestLan),
                               temp.nodes.size());
            m_lanList.push_back(temp);
            list<int>::iterator pos = temp.nodes.begin();
            for ( ; pos != temp.nodes.end(); ++pos)
            {
                m_nodes.insert(make_pair( *pos, --(m_lanList.end()) ));
            }

        }
        catch (EmptyLineException & error)
        {
            // do nothing. Empty lines are OK.
        }
        getline(input, line);
    }
    input.clear();
}

void Graph::partition(std::vector<int> & partitions)
{
    list<Lan>::iterator pos = m_lanList.begin();
    vector<int>::iterator partPos = partitions.begin();
    for ( ; pos != m_lanList.end() && partPos != partitions.end();
          ++pos, ++partPos)
    {
        pos->partition = *partPos;
    }
}

//////////////////////////////////////////////////////
// output
//////////////////////////////////////////////////////

void Graph::assign(int networkSize, int lanSize, int hostSize,
                   int numPartitions) const
{
    assign(cout, networkSize, lanSize, hostSize, numPartitions);
}

void Graph::assign(ostream & output, int networkSize, int lanSize,
            int hostSize, int numPartitions) const
{
#ifdef IP_ASSIGN_DEBUG
    debugAssign(output, networkSize, lanSize, hostSize, numPartitions);
#else
    releaseAssign(output, networkSize, lanSize, hostSize, numPartitions);
#endif
}

//////////////////////////////////////////////////////
// utilities
//////////////////////////////////////////////////////

int Graph::parseLine(string const & line, list<int> & nodes)
{
    int weight = 1;
    size_t i = 0;
    for (i = 0; i < line.size(); ++i)
    {
        char temp = line[i];
        if (!isdigit(temp) && temp != ' ' && temp != '\t'
            && temp != 0x0A && temp != 0x0D)
        {
            throw InvalidCharacterException("Invalid characters in line: "
                                            + line);
        }
    }
    istringstream buffer(line);
    buffer >> weight;
    if (!buffer)
    {
        throw EmptyLineException("Empty line: " + line);
    }
    nodes.clear();
    int temp;
    buffer >> temp;
    while (buffer)
    {
        nodes.push_back(temp);
        buffer >> temp;
    }
    if (nodes.size() < 2)
    {
        throw NotEnoughNodesException("Not enough nodes in lan: " + line);
    }
    return weight;
}

void Graph::convertAddLan(Lan const & lanToAdd, std::vector<int> & neighbors,
                   std::vector<int> & weights) const
{
    list<int>::const_iterator nodePos = lanToAdd.nodes.begin();
    for ( ; nodePos != lanToAdd.nodes.end(); ++nodePos)
    {
        convertAddNode(lanToAdd, *nodePos, neighbors, weights);
    }
}

void Graph::convertAddNode(Lan const & info, int currentNode,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights) const
{
    pair< multimap<int, list<Lan>::iterator>::const_iterator,
        multimap<int, list<Lan>::iterator>::const_iterator> bounds;
    bounds = m_nodes.equal_range(currentNode);
    multimap<int, list<Lan>::iterator>::const_iterator pos;
    for (pos = bounds.first; pos != bounds.second; ++pos)
    {
        if (pos->second->number != info.number)
        {
            neighbors.push_back(pos->second->number);
            weights.push_back(info.weight + pos->second->weight);
        }
    }
}

void Graph::debugAssign(std::ostream & output, int networkSize, int lanSize,
                        int hostSize, int numPartitions) const
{
    vector<int> counter;
    counter.resize(numPartitions);
    fill(counter.begin(), counter.end(), 0);
    cout << networkSize << " " << lanSize << " " << hostSize << endl;
    list<Lan>::const_iterator pos = m_lanList.begin();
    for ( ; pos != m_lanList.end(); ++pos)
    {
        int hostNumber = 1;
        list<int>::const_iterator nodePos = pos->nodes.begin();
        for ( ; nodePos != pos->nodes.end(); ++nodePos)
        {
            cout << pos->number << " " << *nodePos << " ";
            cout << pos->partition << "," << counter[pos->partition]
                 << "," << hostNumber;
            cout << endl;
            ++hostNumber;
        }
        ++(counter[pos->partition]);
    }
}

void Graph::releaseAssign(std::ostream & output, int networkSize, int lanSize,
                          int hostSize, int numPartitions) const
{
    vector<int> counter;
    counter.resize(numPartitions);
    fill(counter.begin(), counter.end(), 0);

    // we want to set up intuitive bit boundaries if possible
    if (networkSize <= 8 && lanSize <= 8 && hostSize <= 8)
    {
        networkSize = 8;
        lanSize = 8;
        hostSize = 8;
    }

    int networkShift = lanSize + hostSize;
    int lanShift = hostSize;
    cout << networkSize << " " << lanSize << " " << hostSize << endl;

    list<Lan>::const_iterator pos = m_lanList.begin();
    for ( ; pos != m_lanList.end(); ++pos)
    {
        int hostNumber = 1;
        list<int>::const_iterator nodePos = pos->nodes.begin();
        for ( ; nodePos != pos->nodes.end(); ++nodePos)
        {
            cout << pos->number << " " << *nodePos << " ";

            bitset<totalBits> result;
            bitset<totalBits> hostNet(hostNumber);
            bitset<totalBits> hostMask;
            hostMask.set();
            hostMask >>= totalBits - hostSize;

            bitset<totalBits> lanNet(counter[pos->partition]);
            lanNet <<= lanShift;
            bitset<totalBits> lanMask;
            lanMask.set();
            lanMask >>= hostSize;
            lanMask <<= prefixBits + networkSize + hostSize;
            lanMask >>= prefixBits + networkSize;

            bitset<totalBits> netNet(pos->partition);
            netNet <<= networkShift;
            bitset<totalBits> netMask;
            netMask.set();
            netMask >>= lanSize + hostSize;
            netMask <<= prefixBits + lanSize + hostSize;
            netMask >>= prefixBits;

            result = (prefix & prefixMask)
                   | (netNet & netMask)
                   | (lanNet & lanMask)
                   | (hostNet & hostMask);

            int i = totalBits - totalBits % 8;
            if (i < totalBits)
            {
                cout << bits2int(result, static_cast<size_t>(i),
                                 static_cast<size_t>(totalBits));
            }

            for (i -= 8; i >= 0; i -= 8)
            {
                if (i < totalBits - 8)
                {
                    cout << '.';
                }
                cout << bits2int(result, static_cast<size_t>(i),
                                 static_cast<size_t>(i + 8));
            }

            cout << endl;
            ++hostNumber;
        }
        ++(counter[pos->partition]);
    }
}

int Graph::bits2int(bitset<totalBits> & bits, size_t start,
                    size_t finish) const
{
    int result = 0;
    int multiplier = 1;
    if (finish > totalBits)
    {
        finish = totalBits;
    }
    if (start < totalBits)
    {
        for (size_t i = start; i < finish; ++i, multiplier *= 2)
        {
            result += static_cast<int>(bits[i]) * multiplier;
        }
    }
    return result;
}

