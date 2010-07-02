// GraphConverter.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef GRAPH_CONVERTER_H_IP_ASSIGN_2
#define GRAPH_CONVERTER_H_IP_ASSIGN_2

// The purpose of GraphConverter is to convert the graph to the
// representation required by METIS.

// GraphConverter uses a GraphData as a representation of the source graph.
// The interface of GraphData must be as follows:

// typedefs inside the class
// LanIterator -- The iterator type for the LAN list representation.
// HostInsideLanIterator -- The iterator type for the host list inside a LAN
// LanInsideHostIterator -- The iterator type for the LAN list inside a host

// methods defined for an instance of the class

// Note that the order for getLanBegin()..getLanEnd() must correspond
// to the LanNumber associated with each LAN. The first must be 0, the
// next 1, and so on.

// LanIterator getLanBegin(void) -- The beginning of the LAN list
// LanIterator getLanEnd(void) -- The end of the LAN list
// int getLanNumber(LanIterator &)
// int getLanNumber(LanInsideHostIterator &)
// int getLanWeight(LanIterator &)
// int getLanWeight(LanInsideHostIterator &)

// HostInsideLanIterator getHostInsideLanBegin(LanIterator &) -- The first
//                                                               host in a LAN
// HostInsideLanIterator getHostInsideLanEnd(LanIterator &)
// LanInsideHostIterator getLanInsideHostBegin(HostInsideLanIterator &)
// LanInsideHostIterator getLanInsideHostEnd(HostInsideLanIterator &)

template <class GraphData, class LanIterator,
    class HostInsideLanIterator,// = GraphData::HostInsideLanIterator,
class LanInsideHostIterator>// = GraphData::LanInsideHostIterator>
class GraphConverter
{
public:
    GraphConverter()
        : data(NULL)
    {
    }

    ~GraphConverter()
    {
    }

    void reset(GraphData & newData)
    {
        data = &newData;
        convert();
    }

    std::vector<int> & getIndexes()
    {
        return indexes;
    }

    std::vector<int> & getNeighbors()
    {
        return neighbors;
    }

    std::vector<int> & getWeights()
    {
        return weights;
    }
private:
    // This object does not own data.
    // data is the graph representation to be used by the algorithm
    // which will be converted to METIS representation.
    GraphData * data;
    std::vector<int> indexes;
    std::vector<int> neighbors;
    std::vector<int> weights;
private:
    void convert(void)
    {
        // ensure that there is no garbage in the output.
        indexes.clear();
        neighbors.clear();
        weights.clear();

        // fill them up with the graph.
        // See the METIS manual, section [5.1 Graph Data Structure], page 18
        LanIterator lanPos = data->getLanBegin();

        for ( ; lanPos != data->getLanEnd(); ++lanPos)
        {
            // neighbors.size() is the total number of edges pushed on so
            // far. This is one greater than the max index. Which is the
            // starting index for that LAN's set of edges.
            indexes.push_back(neighbors.size());
            convertAddLan(lanPos);
        }
        indexes.push_back(neighbors.size());
    }

    void convertAddLan(LanIterator sourceLan)
    {
        HostInsideLanIterator hostPos;
        hostPos = data->getHostInsideLanBegin(sourceLan);

        HostInsideLanIterator hostLimit;
        hostLimit = data->getHostInsideLanEnd(sourceLan);

        // For every host that the source LAN connects to...
        for ( ; hostPos != hostLimit; ++hostPos)
        {
            convertAddHost(sourceLan, hostPos);
        }
    }

    void convertAddHost(LanIterator sourceLan,
                        HostInsideLanIterator currentHost)
    {
        LanInsideHostIterator destPos;
        destPos = data->getLanInsideHostBegin(currentHost);

        LanInsideHostIterator destLimit;
        destLimit = data->getLanInsideHostEnd(currentHost);

        // For every LAN that the current host connects to
        for ( ; destPos != destLimit; ++destPos)
        {
            int sourceNumber = data->getLanNumber(sourceLan);
            int destNumber = data->getLanNumber(destPos);
            // If this is not the same LAN we are searching from
            if (sourceNumber != destNumber && destNumber >= 0)
            {
                // Add this host as a link from the source LAN to the dest LAN.
                neighbors.push_back(destNumber);
                int sourceWeight = data->getLanWeight(sourceLan);
                int destWeight = data->getLanWeight(destPos);
                weights.push_back(sourceWeight + destWeight);
            }
        }
    }
private:
    // No copy construction
    GraphConverter(GraphConverter const &);
    GraphConverter & operator=(GraphConverter const &) { return *this; }
};

#endif
