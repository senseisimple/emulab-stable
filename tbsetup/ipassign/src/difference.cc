// difference.cc

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char * argv[])
{
    if (argc == 3)
    {
        ifstream ideal(argv[1], ios::in);
        ifstream candidate(argv[2], ios::in);
        ofstream relative((argv[2] + string(".relative")).c_str(),
                         ios::out | ios::trunc);
        ofstream absolute((argv[2] + string(".absolute")).c_str(),
                         ios::out | ios::trunc);
        if (ideal && candidate && relative && absolute)
        {
            int left = 0;
            int right = 0;
            size_t total = 1;
            ideal >> left;
            candidate >> right;
            while (ideal && candidate)
            {
                if (right - left < 0)
                {
                    cerr << total << " ideal: " << left << " candidate: " << right << endl;
                }
                absolute << (right - left) << endl;
                double num = right - left;
                double denom = right;
                if (right != 0)
                {
                    relative << (num/denom) << endl;
                }
                ideal >> left;
                candidate >> right;
                ++total;
            }
        }
    }
    return 0;
}
