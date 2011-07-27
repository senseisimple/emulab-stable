// drecal.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include <iomanip>
#include "Exception.h"
#include "SingleSource.h"

using namespace std;

void execute(void);
int parseInput(list< list<int> > & lans);
void addEdges(SingleSource & graph, list< list<int> > & lans);

int main(void)
{
    int result = 0;
    try
    {
        execute();
    }
    catch(exception & error)
    {
        cerr << "apsp: " << error.what() << endl;
        result = 1;
    }
    return result;
}

void execute(void)
{
    // The below initialization is confusing because I want to destroy
    // lans before creating dre. The reason that lans is there in the
    // first place is because a SingleSource requires the number of
    // vertices in the graph at initialization time. Therefore, I need
    // to slurp up all of the input before I initialize
    // SingleSource. This goes into lans, and lans is destroyed before
    // the big memory user is created. As n gets big, page swapping
    // will become the big issue. I want to delay that a bit if
    // possible.
    auto_ptr<SingleSource> graphPtr;
    {
        int greatestVertex = -1;
        list< list<int> > lans;
        greatestVertex = parseInput(lans);
        graphPtr.reset(new SingleSource(greatestVertex + 1));
        addEdges(*graphPtr, lans);
    }
    // The reset above is always executed. If the new fails, an
    // exception is thrown. Therefore the following statement is safe.
    SingleSource & graph(*graphPtr);
    for (int i = 0; i < graph.getVertexCount(); ++i)
    {
        graph.route(i);
        for (int j = 0; j < graph.getVertexCount(); ++j)
        {
            cout << graph.getDistance(j) << endl;
        }
    }
}

int parseInput(list< list<int> > & lans)
{
    int greatestVertex = -1;
    string lineString;
    getline(cin, lineString);
    while (cin)
    {
        istringstream line(lineString);
        int temp = 0;
        lans.push_back(list<int>());
        // The first two are weight and size. We can ignore that here.
        line >> temp;
        line >> temp;
        // Then there are n numbers representing the vertices. Stuff
        // them into the list.
        int current = 0;
        line >> current;
        while (line)
        {
            greatestVertex = max(greatestVertex, current);
            lans.back().push_back(current);
            line >> current;
        }
        getline(cin, lineString);
    }
    return greatestVertex;
}

void addEdges(SingleSource & graph, list< list<int> > & lans)
{
    list< list<int> >::iterator lanPos = lans.begin();
    list< list<int> >::iterator lanLimit = lans.end();
    for ( ; lanPos != lanLimit; ++lanPos)
    {
        // Now we add the edges into the graph. Note that the graph is
        // normal (non-hyper), so we add pairwise selections from the
        // vertices we've just inputted.
        list<int>::iterator leftPos = lanPos->begin();
        list<int>::iterator limit = lanPos->end();
        for ( ; leftPos != limit; ++leftPos)
        {
            list<int>::iterator rightPos = leftPos;
            ++rightPos;
            for ( ; rightPos != limit; ++rightPos)
            {
                graph.insertEdge(*leftPos, *rightPos, 1);
            }
        }
    }
}
