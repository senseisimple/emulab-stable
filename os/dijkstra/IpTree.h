// IpTree.h

#ifndef IP_TREE_H_DISTRIBUTED_DIJKSTRA_1
#define IP_TREE_H_DISTRIBUTED_DIJKSTRA_1

enum { IP_SIZE = 32 };

class IpTree
{
public:
    IpTree()
        : m_firstHop(0)
        , m_depth(0)
    {
    }

    void reset(void)
    {
        m_child[0].reset(NULL);
        m_child[1].reset(NULL);
        m_firstHop = 0;
        m_childHops.clear();
        m_depth = 0;
    }

    void addRoute(IPAddress ip, int newFirstHop, int depth = 0)
    {
        m_depth = depth;
        if (depth < IP_SIZE)
        {
            IPAddress bit = (ip >> (IP_SIZE - depth - 1)) & 0x01;
            if (m_child[bit].get() == NULL)
            {
                m_child[bit].reset(new IpTree());
            }
            m_child[bit]->addRoute(ip, newFirstHop, depth + 1);
        }
        else
        {
            m_firstHop = newFirstHop;
        }
        ++(m_childHops[newFirstHop]);
    }

    void printRoutes(int parentFirstHop, HostHostToIpMap const & ip,
                     int source, IPAddress subnet)
    {
        if (m_childHops.empty())
        {
            return;
        }

        map<int,int>::iterator mostPos = findMostPos();

        if (mostPos->first != parentFirstHop)
        {
            m_firstHop = mostPos->first;
            map< int, pair<string,string> >::const_iterator pos;
            pos = ip[source].find(m_firstHop);
            if (pos != ip[source].end())
            {
                printRouteToSubnet(pos->second.first, pos->second.second,
                                   ipToString(subnet << (IP_SIZE - m_depth)),
                                   m_depth, 1);
                // TODO: replace hardwired '1' with actual cost metric
            }
            else
            {
                throw StringException("Internal error: Corruption in data structures: Compressor::add()");
            }
        }

        if (m_child[0].get() != NULL)
        {
            m_child[0]->printRoutes(mostPos->first, ip, source, subnet << 1);
        }

        if (m_child[1].get() != NULL)
        {
            m_child[1]->printRoutes(mostPos->first, ip, source,
                                    (subnet << 1) + 1);
        }
    }

    map<int,int>::iterator findMostPos(void)
    {
        map<int,int>::iterator pos = m_childHops.begin();
        map<int,int>::iterator mostPos = pos;
        int mostScore = mostPos->second;
        ++pos;
        while (pos != m_childHops.end())
        {
            if (pos->second > mostScore)
            {
                mostPos = pos;
                mostScore = mostPos->second;
            }
            ++pos;
        }
        return mostPos;
    }
private:
    IpTree(IpTree const &);
    IpTree & operator=(IpTree const &) { return *this; }
private:
    auto_ptr<IpTree> m_child[2];
    int m_firstHop;
    map<int, int> m_childHops;
    int m_depth;
};

class Compressor
{
private:
    static const int shift_10 = IP_SIZE - 8;
    static const IPAddress ip_10 = 10;

    static const int shift_172_16 = IP_SIZE - 12;
    static const IPAddress ip_172_16 = (172 << 8) + 16;

    static const int shift_192_168 = IP_SIZE - 16;
    static const IPAddress ip_192_168 = (192 << 8) + 168;
public:
    Compressor()
        : graph(NULL)
        , ipMap(NULL)
    {
    }

    ~Compressor()
    {
    }

    void compress(SingleSource const & newGraph, HostHostToIpMap const & newIp)
    {
        graph = &newGraph;
        ipMap = &newIp;
        root_10.reset();
        root_172_16.reset();
        root_192_168.reset();

        std::set<std::string> implicitIp;
        std::multimap< int,pair<string,string> >::const_iterator implicitPos;
        std::multimap< int,pair<string,string> >::const_iterator implicitLimit;
        implicitPos = (*ipMap)[graph->getSource()].begin();
        implicitLimit = (*ipMap)[graph->getSource()].end();
        for ( ; implicitPos != implicitLimit; ++implicitPos)
        {
            implicitIp.insert(implicitPos->second.second);
        }

        for (int i = 0; i < graph->getVertexCount(); ++i)
        {
            int dest = i;
            if (dest != graph->getSource())
            {
                std::multimap< int, pair<string,string> >::const_iterator pos;
               std::multimap< int, pair<string,string> >::const_iterator limit;
                pos = (*ipMap)[dest].begin();
                limit = (*ipMap)[dest].end();
                for ( ; pos != limit; ++pos)
                {
                    if (implicitIp.find(pos->second.first) == implicitIp.end())
                    {
                        IPAddress destIp = stringToIP(pos->second.first);
                        add(dest, destIp);
                    }
                }
            }
        }
        root_10.printRoutes(INT_MAX, *ipMap, graph->getSource(), ip_10);
        root_172_16.printRoutes(INT_MAX, *ipMap, graph->getSource(),
                                ip_172_16);
        root_192_168.printRoutes(INT_MAX, *ipMap, graph->getSource(),
                                 ip_192_168);
    }
private:
    void add(int dest, IPAddress destIp)
    {
        int firstHop = graph->getFirstHop(dest);
        if ((destIp >> shift_10) == ip_10)
        {
            root_10.addRoute(destIp, firstHop, IP_SIZE - shift_10);
        }
        else if ((destIp >> shift_172_16) == ip_172_16)
        {
            root_172_16.addRoute(destIp, firstHop, IP_SIZE - shift_172_16);
        }
        else if ((destIp >> shift_192_168) == ip_192_168)
        {
            root_192_168.addRoute(destIp, firstHop, IP_SIZE - shift_192_168);
        }
        else
        {
            map< int, pair<string,string> >::const_iterator pos;
            pos = (*ipMap)[graph->getSource()].find(firstHop);
            if (pos != (*ipMap)[graph->getSource()].end())
            {
                // TODO: Figure out what cost means
                printRouteToIp(pos->second.first, pos->second.second,
                               ipToString(destIp), 1);
            }
            else
            {
                throw StringException("Internal error: Corruption in data structures: Compressor::add()");
            }
        }
    }
private:
    Compressor(Compressor const &);
    Compressor & operator=(Compressor const &) { return *this; }
private:
    IpTree root_10;
    IpTree root_172_16;
    IpTree root_192_168;

    SingleSource const * graph;
    HostHostToIpMap const * ipMap;
};

#endif
