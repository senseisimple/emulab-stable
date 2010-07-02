#include <stdio.h>
#include<iostream>
#include<cstdlib>
#include<vector>
#include<fstream>
#include "dc.hh"

int main(int argc, char **argv)
{
    std::ifstream fileHandle;

    fileHandle.open(argv[1], std::ios::in);
    std::vector<double> delays;
    int delayVal;

    while(!(fileHandle.eof()))
    {
        fileHandle >> delayVal;
        if((fileHandle.eof()))
            break;
        delays.push_back(delayVal);
    }

    fileHandle.close();

    delay_correlation(delays, delays.size());
}

