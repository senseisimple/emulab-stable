// route-histogram.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This program is a filter from cin to cout. It creates a histogram
// out of the input and puts it in the output.

#include <string>
#include <fstream>
#include <vector>
#include <list>
#include <sstream>
#include <iostream>
#include <map>

#include "Exception.h"

using namespace std;

namespace arg
{
    int bucketCount = 11;
}

class InvalidInputException : public StringException
{
public:
    explicit InvalidInputException(std::string const & error)
        : StringException("I don't know what you mean by: " + error)
    {
    }
    virtual ~InvalidInputException() throw() {}
};

void parseArgs(int argc, char * argv[]);
void execute(void);
void setupRanges(int maxCount, map<int, size_t> & ranges);
int getRange(int bucket, int bucketCount, int maxValue);
void fillBuckets(list<int> const & countList,
                 map<int, size_t> const & ranges,
                 vector<int> & buckets);
void printBuckets(map<int, size_t> const & ranges,
                 vector<int> const & buckets);

int main(int argc, char * argv[])
{
    int result = 0;
    try
    {
        parseArgs(argc, argv);
        execute();
    }
    catch (exception & error)
    {
        cerr << "route-histogram: " << error.what() << endl;
        result = 1;
    }
    return result;
}

void parseArgs(int argc, char * argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--bucket-count=",
                    sizeof("--bucket-count=") - 1) == 0)
        {
            int newBucketCount = 0;
            istringstream buffer(argv[i] + sizeof("--bucket-count=") - 1);
            buffer >> newBucketCount;
            if (!buffer)
            {
                throw InvalidArgumentException(argv[i]);
            }
            arg::bucketCount = newBucketCount;
        }
        else
        {
            throw InvalidArgumentException(argv[i]);
        }
    }
}


void execute(void)
{
    int maxCount = 0;
    list<int> countList;
    string bufferString;
    getline(cin, bufferString);
    while (cin)
    {
        istringstream buffer(bufferString);
        int currentCount = 0;
        buffer >> currentCount;
        if (!buffer)
        {
            throw InvalidInputException(bufferString);
        }

        maxCount = max(maxCount, currentCount);
        countList.push_back(currentCount);

        getline(cin, bufferString);
    }

    map<int, size_t> ranges;
    setupRanges(maxCount, ranges);

    vector<int> buckets;
    buckets.resize(arg::bucketCount);
    fill(buckets.begin(), buckets.end(), 0);

    fillBuckets(countList, ranges, buckets);
    printBuckets(ranges, buckets);
}

void setupRanges(int maxCount, map<int, size_t> & ranges)
{
    ranges[0] = 0;
    size_t i = 1;
    for ( ; i < static_cast<size_t>(arg::bucketCount + 1); ++i)
    {
        ranges[getRange(static_cast<int>(i), arg::bucketCount,
                        maxCount)] = i-1;
    }
}

int getRange(int bucket, int bucketCount, int maxValue)
{
    int result = 0;
    if (bucket == 0)
    {
        result = 0;
    }
    else if (bucket < bucketCount)
    {
        int num = bucket * maxValue;
        int denom = bucketCount;
        // round down on the division.
        result = num/denom;
    }
    else if (bucket = bucketCount)
    {
        result = maxValue;
    }
    else
    {
        result = 0;
    }
    return result;
}

void fillBuckets(list<int> const & countList,
                 map<int, size_t> const & ranges,
                 vector<int> & buckets)
{
    list<int>::const_iterator pos = countList.begin();
    list<int>::const_iterator limit = countList.end();
    for ( ; pos != limit; ++pos)
    {
        map<int, size_t>::const_iterator maxPos = ranges.lower_bound(*pos);
        ++(buckets[maxPos->second]);
    }
}

void printBuckets(map<int, size_t> const & ranges,
                 vector<int> const & buckets)
{
    map<int, size_t>::const_iterator pos = ranges.begin();
    map<int, size_t>::const_iterator forward = pos;
    ++forward;
    map<int, size_t>::const_iterator limit = ranges.end();
    while(forward != limit)
    {
        cout << pos->first << " " << forward->first << " "
             << buckets[forward->second] << endl;

        ++pos;
        ++forward;
    }
}
