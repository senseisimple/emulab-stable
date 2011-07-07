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

void sgenrand(unsigned long seed);
unsigned long genrand();

namespace arg
{
    bool doSample = false;
    unsigned long seed = 1;
}

// This is a staggered matrix which looks like this:
//
// x
// xx
// xxx
// xxxx
// ...
//
// The row should always be higher than the column.
class AllPairs
{
public:
    AllPairs(int vertexCount = 0)
    {
        reset(vertexCount);
    }

    ~AllPairs()
    {
    }

    void reset(int vertexCount = 0)
    {
        data.resize(vertexCount);
        for (size_t i = 0; i < static_cast<size_t>(vertexCount); ++i)
        {
            data[i].resize(i+1);
            fill(data[i].begin(), data[i].end(), 0);
        }
    }

    int & operator() (size_t first, size_t second)
    {
        return data[max(first, second)][min(first, second)];
    }

    void printPairs(void)
    {
        cout << setprecision(4) << setiosflags(ios::fixed | ios::showpoint);
        for (size_t i = 0; i < data.size(); ++i)
        {
            for (size_t j = 0; j < data[i].size(); ++j)
            {
                if (i != j)
                {
                    double score = static_cast<double>(data[i][j])/
                                   static_cast<double>(data.size());
                    cout << "( " << i << " , " << j << " ) " << score << endl;
                }
            }
        }
    }
private:
    AllPairs(AllPairs &);
    AllPairs & operator=(AllPairs const &) { return *this; }
private:
    vector< vector<int> > data;

};

void processArgs(int argc, char * argv[]);
void execute(void);
int parseInput(list< list<int> > & lans);
void addEdges(SingleSource & graph, list< list<int> > & lans);
void sampleDre(vector< vector<int> > const & firstHops, AllPairs & dre);
void completeDre(vector< vector<int> > const & firstHops, AllPairs & dre);
int randomVertex(int count);

int main(int argc, char * argv[])
{
    int result = 0;
    try
    {
        processArgs(argc, argv);
        sgenrand(arg::seed);
        execute();
    }
    catch(exception & error)
    {
        cerr << argv[0] << ": " << error.what() << endl;
        result = 1;
    }
    return result;
}

void processArgs(int argc, char * argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--sample", sizeof("--sample") - 1) == 0)
        {
            arg::doSample = true;
        }
        else if (strncmp(argv[i], "--complete", sizeof("--complete") - 1) == 0)
        {
            arg::doSample = false;
        }
        else
        {
            throw InvalidArgumentException(argv[i]);
        }
    }
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
    AllPairs dre(graph.getVertexCount());
    vector< vector<int> > firstHops;
    firstHops.resize(graph.getVertexCount());
    for (int i = 0; i < graph.getVertexCount(); ++i)
    {
        graph.route(i);
        firstHops[i] = graph.getAllFirstHops();
    }
    if (arg::doSample)
    {
        sampleDre(firstHops, dre);
    }
    else
    {
        completeDre(firstHops, dre);
    }
    dre.printPairs();
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

void checkTriplet(int source, int dest1, int dest2,
                  vector< vector<int> > const & firstHops,
                  AllPairs & dre, int increment)
{
    if (firstHops[source][dest1] == firstHops[source][dest2])
    {
        dre(dest1, dest2) += increment;
    }
}

void sampleDre(vector< vector<int> > const & firstHops, AllPairs & dre)
{
    int limit = static_cast<int>(sqrt(static_cast<double>(firstHops.size())));
    for (int dest1 = 0; dest1 < firstHops.size(); ++dest1)
    {
        for (int dest2 = dest1 + 1; dest2 < firstHops.size(); ++dest2)
        {
            for (int i = 0; i < limit; ++i)
            {
                int source = randomVertex(firstHops.size());
                checkTriplet(source, dest1, dest2, firstHops, dre, limit);
            }
        }
    }
}

void completeDre(vector< vector<int> > const & firstHops, AllPairs & dre)
{
    for (int source = 0; source < firstHops.size(); ++source)
    {
        for (int dest1 = 0; dest1 < firstHops.size(); ++dest1)
        {
            for (int dest2 = dest1 + 1; dest2 < firstHops.size(); ++dest2)
            {
                checkTriplet(source, dest1, dest2, firstHops, dre, 1);
            }
        }
    }
}

int randomVertex(int count)
{
    return abs(static_cast<int>(genrand())) % count;
}
