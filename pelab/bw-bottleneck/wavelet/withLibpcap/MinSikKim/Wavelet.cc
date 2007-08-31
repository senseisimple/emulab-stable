#include<iostream>
#include<cstdlib>
#include<vector>
#include<fstream>
#include "dc.hh"

int main(int argc, char **argv)
{
    std::ifstream fileHandle;

    fileHandle.open(argv[1], std::ios::in);
    std::vector<double> delays1, delays2;
    int delayVal1, delayVal2;

    while(!(fileHandle.eof()))
    {
        fileHandle >> delayVal1;
        if((fileHandle.eof()))
            break;
        fileHandle >> delayVal2;

        delays1.push_back(delayVal1);
        delays2.push_back(delayVal2);
    }

    fileHandle.close();

    bool retVal = delay_correlation(delays1, delays2, delays1.size());

    std::cout<<"Correlation = "<<retVal<<std::endl;

}

