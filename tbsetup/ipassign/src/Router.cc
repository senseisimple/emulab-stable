// Router.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Router.h"

using namespace std;

Router::~Router()
{
}

void Router::reset(Assigner const & newAssign)
{
    newAssign.graph(m_nodeToLevel, m_levelMaskSize, m_levelPrefix,
                    m_levelMakeup, m_lanWeights);
}

bool Router::isValidNode(size_t node) const
{
    // [0] is the LAN level and [node] is which LAN that node belongs to.
    return !(m_nodeToLevel[0][node].empty());
}
