// PTree.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef PTREE_H_IP_ASSIGN_2
#define PTREE_H_IP_ASSIGN_2

namespace ptree
{
/**************************************************/

    class ConflictingNetworkEntryException : public StringException
    {
    public:
        explicit ConflictingNetworkEntryException(IPAddress prefix,
                                                  IPAddress netMask)
            : StringException("Conflicting network entry given to a ptree"
                              " node: IP(" + ipToString(prefix) + "), "
                              " netmask(" + ipToString(netMask) + ")")
        {
        }
    };

/**************************************************/

    class AddChildToLeafException : public StringException
    {
    public:
        explicit AddChildToLeafException(void)
            : StringException("An attempt was made to add a child to a ptree"
                              " Leaf node")
        {
        }
    };

/**************************************************/

    class Visitor;

    class Node
    {
    public:
        Node() : m_prefix(0), m_maskSize(0), m_visitCount(0) {}
        virtual ~Node() {}

        virtual std::auto_ptr<Node> clone(void)=0;
        virtual std::auto_ptr<Node> clone(Node * newParent)=0;
        virtual Node * getParent(void)=0;
        virtual Node * getChild(void)=0;
        virtual Node * getSibling(void)=0;
        virtual void setParent(Node * newParent)=0;
        virtual void addChild(std::auto_ptr<Node> newChild)=0;
        virtual void addSibling(std::auto_ptr<Node> newSibling)=0;

        IPAddress getPrefix(void) const
        {
            return m_prefix;
        }

        int getMaskSize(void) const
        {
            return m_maskSize;
        }

        void setNetworkEntry(IPAddress newPrefix, int newMaskSize)
        {
            IPAddress mask = maskSizeToMask(newMaskSize);
            if ((newPrefix & mask) != newPrefix)
            {
                throw ConflictingNetworkEntryException(newPrefix, mask);
            }
            m_prefix = newPrefix;
            m_maskSize = newMaskSize;
        }

        std::list<size_t>::const_iterator getNewInternalHostsBegin(void) const
        {
            return m_newInternalHosts.begin();
        }

        std::list<size_t>::const_iterator getNewInternalHostsEnd(void) const
        {
            return m_newInternalHosts.end();
        }

        void addNewInternalHost(size_t newNode)
        {
            m_newInternalHosts.push_back(newNode);
        }

        void clearNewInternalHosts(void)
        {
            m_newInternalHosts.clear();
        }

        void addVisit(void)
        {
            ++m_visitCount;
        }

        void removeVisit(void)
        {
            --m_visitCount;
        }

        int getVisit(void)
        {
            return m_visitCount;
        }

        virtual void accept(Visitor & target)=0;

        void logStructure(std::ostream & output, std::vector<bool> & table)
        {
            // print out the current node
            output << "PARSE_TREE: ";
            for (size_t i = 0; i < table.size(); ++i)
            {
                output << " ";
                if (table[i])
                {
                    output << "|";
                }
                else
                {
                    output << " ";
                }
                output << "  ";
            }
            output << " +->(" << ipToString(m_prefix) << "/" << m_maskSize
                   << ") ";
            printCustomDebug(output);
            output << "\n";

            // print out the child nodes
            if (getChild() != NULL)
            {
                if (getSibling() == NULL)
                {
                    table.push_back(false);
                }
                else
                {
                    table.push_back(true);
                }
                getChild()->logStructure(output, table);
                table.pop_back();
            }

            // print out the sibling nodes
            if (getSibling() != NULL)
            {
                getSibling()->logStructure(output, table);
            }
        }

        virtual void printCustomDebug(std::ostream & output)=0;
    protected:
        void copyTo(Node & dest) const
        {
            dest.m_prefix = m_prefix;
            dest.m_maskSize = m_maskSize;
        }
    private:
        // All copies must be polymorphic through clone()
        Node(Node const & right) {}
        Node & operator=(Node const & right) { return *this; }
    private:
        IPAddress m_prefix;
        int m_maskSize;
        std::list<size_t> m_newInternalHosts;
        int m_visitCount;
    };

/**************************************************/

    class Branch;
    class Leaf;

    class Visitor
    {
    public:
        virtual ~Visitor() {}
        virtual void visitBranch(Branch & target)=0;
        virtual void visitLeaf(Leaf & target)=0;
    };

/**************************************************/

    class Branch : public Node
    {
    public:
        Branch() : m_parent(NULL) {}
        virtual ~Branch() {}

        virtual std::auto_ptr<Node> clone(void)
        {
            return clone(NULL);
        }

        virtual std::auto_ptr<Node> clone(Node * newParent)
        {
            std::auto_ptr<Branch> copy(new Branch());
            copy->m_parent = newParent;
            if (m_child.get() != NULL)
            {
                copy->m_child.reset(m_child->clone(this).release());
            }
            if (m_sibling.get() != NULL)
            {
                copy->m_sibling.reset(m_sibling->clone(newParent).release());
            }

            Node::copyTo(*copy);

            std::auto_ptr<Node> result(copy.release());
            return result;
        }

        virtual Node * getParent(void)
        {
            return m_parent;
        }

        virtual Node * getChild(void)
        {
            return m_child.get();
        }

        virtual Node * getSibling(void)
        {
            return m_sibling.get();
        }

        virtual void setParent(Node * newParent)
        {
            m_parent = newParent;
        }

        virtual void addChild(std::auto_ptr<Node> newChild)
        {
            if (newChild.get() != NULL)
            {
                newChild->setParent(this);
                if (m_child.get() == NULL)
                {
                    m_child = newChild;
                }
                else
                {
                    // The new child goes at the front of the list.
                    swap_auto_ptr(m_child, newChild);
                    m_child->addSibling(newChild);
                }
            }
        }

        virtual void addSibling(std::auto_ptr<Node> newSibling)
        {
            if (newSibling.get() != NULL)
            {
                newSibling->setParent(m_parent);
                if (m_sibling.get() == NULL)
                {
                    m_sibling = newSibling;
                }
                else
                {
                    m_sibling->addSibling(newSibling);
                }
            }
        }

        virtual void accept(Visitor & target)
        {
            target.visitBranch(*this);
        }
        virtual void printCustomDebug(std::ostream & output)
        {
            output << "Branch";
        }
    private:
        // All copies must be polymorphic through clone()
        Branch(Branch const & right) {}
        Branch & operator=(Branch const & right) { return *this; }
    private:
        Node * m_parent;
        std::auto_ptr<Node> m_child;
        std::auto_ptr<Node> m_sibling;
    };

/**************************************************/

    class Leaf : public Node
    {
    public:
        Leaf() : m_parent(NULL), m_lanIndex(0) {}
        virtual ~Leaf() {}

        virtual std::auto_ptr<Node> clone(void)
        {
            return clone(NULL);
        }

        virtual std::auto_ptr<Node> clone(Node * newParent)
        {
            std::auto_ptr<Leaf> copy(new Leaf());
            copy->m_parent = newParent;
            copy->m_lanIndex = m_lanIndex;

            if (m_sibling.get() != NULL)
            {
                copy->m_sibling.reset(m_sibling->clone(newParent).release());
            }

            Node::copyTo(*copy);

            std::auto_ptr<Node> result(copy.release());
            return result;
        }

        virtual Node * getParent(void)
        {
            return m_parent;
        }

        virtual Node * getChild(void)
        {
            return NULL;
        }

        virtual Node * getSibling(void)
        {
            return m_sibling.get();
        }

        virtual void setParent(Node * newParent)
        {
            m_parent = newParent;
        }

        virtual void addChild(std::auto_ptr<Node> newChild)
        {
            throw AddChildToLeafException();
        }

        virtual void addSibling(std::auto_ptr<Node> newSibling)
        {
            if (newSibling.get() != NULL)
            {
                newSibling->setParent(m_parent);
                if (m_sibling.get() == NULL)
                {
                    m_sibling = newSibling;
                }
                else
                {
                    m_sibling->addSibling(newSibling);
                }
            }
        }

        void setLanIndex(size_t newLanIndex)
        {
            m_lanIndex = newLanIndex;
        }

        size_t getLanIndex(void)
        {
            return m_lanIndex;
        }

        virtual void accept(Visitor & target)
        {
            target.visitLeaf(*this);
        }
        virtual void printCustomDebug(std::ostream & output)
        {
            output << "Leaf: " << m_lanIndex;
        }
    private:
        // All copies must be polymorphic through clone()
        Leaf(Leaf const & right) {}
        Leaf & operator=(Leaf const & right) { return *this; }
    private:
        Node * m_parent;
        std::auto_ptr<Node> m_sibling;
        size_t m_lanIndex;
    };

/**************************************************/

    struct LeafLan
    {
        LeafLan() : number(0), weight(0), partition(NULL)
        {
        }
        size_t number;
        int weight;
        Node * partition;
        std::vector<size_t> hosts;
    };

/**************************************************/

    class ParentVisitor : public Visitor
    {
    public:
        virtual ~ParentVisitor() {}

        void visitBranch(Branch & target)
        {
            if (visitNode(target) && target.getParent() != NULL)
            {
                target.getParent()->accept(*this);
            }
        }

        void visitLeaf(Leaf & target)
        {
            if (visitNode(target) && target.getParent() != NULL)
            {
                target.getParent()->accept(*this);
            }
        }

        virtual bool visitNode(Node & target)=0;
    };

/**************************************************/

    class SiblingListVisitor : public Visitor
    {
    public:
        SiblingListVisitor(std::list<Node *> & newSiblingList)
            : siblingList(newSiblingList) {}
        virtual ~SiblingListVisitor() {}
        virtual void visitBranch(Branch & target)
        {
            visitNode(target);
        }

        virtual void visitLeaf(Leaf & target)
        {
            visitNode(target);
        }

        void visitNode(Node & target)
        {
            siblingList.push_back(&target);
            if (target.getSibling() != NULL)
            {
                target.getSibling()->accept(*this);
            }
        }

    private:
        std::list<Node *> & siblingList;
    };

/**************************************************/
}

#endif
