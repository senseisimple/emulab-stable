// HierarchicalAssigner.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This assigner uses its Partitioner to recursively divide the graph to
// create a hierarchy.

#ifndef HIERARCHICAL_ASSIGNER_H_IP_ASSIGN_2
#define HIERARCHICAL_ASSIGNER_H_IP_ASSIGN_2

#include "Assigner.h"
#include "GraphConverter.h"

class HierarchicalAssigner : public Assigner
{
public:
    HierarchicalAssigner(Partition & newPartition);
//    HierarchicalAssigner(HierarchicalAssigner const & right);
    virtual ~HierarchicalAssigner();

    virtual std::auto_ptr<Assigner> clone(void) const;

    virtual void setPartition(Partition & newPartition);
    virtual void addLan(int bits, int weight, std::vector<size_t> nodes);
    virtual void ipAssign(void);
    virtual void print(std::ostream & output) const;
    virtual void getGraph(std::auto_ptr<ptree::Node> & outTree,
                          std::vector<ptree::LeafLan> & outLans);
    virtual NodeLookup & getHosts(void);
private:
    void divideAndConquer(std::auto_ptr<ptree::Branch> & parent,
                          IPAddress subnet, int blockBits,
                          std::list<size_t> & partitionList);
    void initPartition(ptree::Node * parent,
                       std::list<size_t> & partitionList);
    bool partitionPartition(ptree::Node * parent, int blockBits,
                            std::list<size_t> & partitionList,
                            std::vector< std::list<size_t> > & components);
    bool partitioningIsOk(int blockBits, std::vector<int> & partitioning,
                          std::list<size_t> & partitionList);
    // This is an adapter which mediates our data and the requirements
    // of the conversion algorithm. See GraphConverter.h for the
    // specification.
    class HierarchicalGraphData
    {
    private:
        static const int NUMBER_ERROR = -1;
    public:
        typedef std::list<size_t>::const_iterator LanIterator;
        typedef std::vector<size_t>::const_iterator HostInsideLanIterator;
        typedef std::vector<size_t>::const_iterator LanInsideHostIterator;
    public:
        HierarchicalGraphData(std::vector<ptree::LeafLan> const & newLans,
                              NodeLookup const & newHosts,
                              std::vector<int> const & newLanVertexNumbers)
            : currentPartition(NULL)
            , usedLans(NULL)
            , lans(newLans)
            , hosts(newHosts)
            , lanVertexNumbers(newLanVertexNumbers)
        {
        }

        ~HierarchicalGraphData()
        {
        }

        void reset(ptree::Node * newPartition,
                   std::list<size_t> const & newUsedLans)
        {
            currentPartition = newPartition;
            usedLans = &newUsedLans;
        }

        LanIterator getLanBegin(void)
        {
            if (usedLans != NULL)
            {
                return usedLans->begin();
            }
            else
            {
                throw StringException("HierarchicalGraphData accessed before usedLans was initialized");
            }
        }

        LanIterator getLanEnd(void)
        {
            if (usedLans != NULL)
            {
                return usedLans->end();
            }
            else
            {
                throw StringException("HierarchicalGraphData accessed before usedLans was initialized");
            }
        }

        int getLanNumber(LanIterator & currentLan)
        {
            if (lans[*currentLan].partition == currentPartition)
            {
                return lanVertexNumbers[*currentLan];
            }
            else
            {
                return NUMBER_ERROR;
            }
        }

        int getLanNumber(LanInsideHostIterator & currentLan)
        {
            if (lans[*currentLan].partition == currentPartition)
            {
                return lanVertexNumbers[*currentLan];
            }
            else
            {
                return NUMBER_ERROR;
            }
        }

        int getLanWeight(LanIterator & currentLan)
        {
            return lans[*currentLan].weight;
        }

        int getLanWeight(LanInsideHostIterator & currentLan)
        {
            return lans[*currentLan].weight;
        }

        HostInsideLanIterator getHostInsideLanBegin(LanIterator & currentLan)
        {
            return lans[*currentLan].hosts.begin();
        }

        HostInsideLanIterator getHostInsideLanEnd(LanIterator & currentLan)
        {
            return lans[*currentLan].hosts.end();
        }

        LanInsideHostIterator getLanInsideHostBegin(
                                           HostInsideLanIterator & currentHost)
        {
            return hosts[*currentHost].begin();
        }

        LanInsideHostIterator getLanInsideHostEnd(
                                           HostInsideLanIterator & currentHost)
        {
            return hosts[*currentHost].end();
        }
    private:
        ptree::Node * currentPartition;
        std::list<size_t> const * usedLans;
        std::vector<ptree::LeafLan> const & lans;
        NodeLookup const & hosts;
        std::vector<int> const & lanVertexNumbers;
    private:
        HierarchicalGraphData(HierarchicalGraphData const &);
        HierarchicalGraphData & operator=(HierarchicalGraphData const &)
            { return *this; }
    };
private:
    // Information about each LAN.
    std::vector<ptree::LeafLan> lans;

    // Used for dividing.
    std::vector<int> lanVertexNumbers;

    // Given a level number and a node number, which partitions/LANs
    // does that node belong to?
    NodeLookup hosts;

    // This will hold ther result of the partitioning/assignment
    std::auto_ptr<ptree::Node> tree;

    // !!! IMPORTANT !!!
    // The declaration for 'convertAdapter' must come after the
    // declarations of 'lans', 'lanVertexNumbers', and 'hosts'.
    // The declaration for 'convertAdapter' must come before the
    // declaration of 'converter'
    // This is because of an obscure C++ rule about initialization.

    // This is the adapter which converts our data into a form usable for
    // converter
    HierarchicalGraphData convertAdapter;

    // This object can be used to convert from any data structure
    // to the METIS representation. There need be only one
    // in the class(in spite of recursion), because it is reset and
    // dispensed with before each recursion. No frame of the recursion
    // needs it after that frame's recursive call.
    GraphConverter<HierarchicalGraphData, HierarchicalGraphData::LanIterator,
        HierarchicalGraphData::HostInsideLanIterator,
        HierarchicalGraphData::LanInsideHostIterator> converter;

    // The pointer to the polymorphic partition object. We use this object
    // to determine partitioning.
    Partition * m_partition;
};

#endif
