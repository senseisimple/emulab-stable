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
#include <iomanip>

#include "Exception.h"

using namespace std;

namespace arg
{
    bool fixedSizeBuckets = false;
    int bucketSize = 10;
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
void setupRanges(int maxCount, map<int, int> & ranges);
int getRange(int bucket, int bucketCount, int maxValue);
void fillBuckets(list<int> const & countList,
                 map<int, int> & ranges);
void printBuckets(map<int, int> const & ranges);

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
        else if (strncmp(argv[i], "--bucket-size=",
                    sizeof("--bucket-size=") - 1) == 0)
        {
            int newBucketSize = 0;
            istringstream buffer(argv[i] + sizeof("--bucket-size=") - 1);
            buffer >> newBucketSize;
            if (!buffer)
            {
                throw InvalidArgumentException(argv[i]);
            }
            arg::bucketSize = newBucketSize;
        }
        else if (strncmp(argv[i], "--fixed-size-buckets",
                    sizeof("--fixed-size-buckets") - 1) == 0)
        {
            arg::fixedSizeBuckets = true;
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

    map<int, int> ranges;
    setupRanges(maxCount, ranges);
    fillBuckets(countList, ranges);
    printBuckets(ranges);
}

void setupRanges(int maxCount, map<int, int> & ranges)
{
    ranges[0] = 0;
    if (arg::fixedSizeBuckets)
    {
        ranges[1] = 0;
        ranges[2] = 0;
        ranges[3] = 0;
        arg::bucketCount = maxCount / arg::bucketSize;
        if (maxCount % arg::bucketSize != 0)
        {
            ++(arg::bucketCount);
        }
    }
    int i = 1;
    for ( ; i < arg::bucketCount + 1; ++i)
    {
        ranges[getRange(i, arg::bucketCount, maxCount)] = 0;
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
        if (arg::fixedSizeBuckets)
        {
            result = bucket * arg::bucketSize;
        }
        else
        {
            int num = bucket * maxValue;
            int denom = bucketCount;
            // round down on the division.
            result = num/denom;
        }
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
                 map<int, int> & ranges)
{
    list<int>::const_iterator pos = countList.begin();
    list<int>::const_iterator limit = countList.end();
    for ( ; pos != limit; ++pos)
    {
        map<int, int>::iterator maxPos = ranges.lower_bound(*pos);
        ++(maxPos->second);
    }
}

void printBuckets(map<int, int> const & ranges)
{
    map<int, int>::const_iterator pos = ranges.begin();
    map<int, int>::const_iterator forward = pos;
    ++forward;
    map<int, int>::const_iterator limit = ranges.end();
    while(forward != limit)
    {
        cout << setw(4) << pos->first << " "
             << setw(4) << forward->first << " "
             << setw(6) << forward->second << endl;

        ++pos;
        ++forward;
    }
}
