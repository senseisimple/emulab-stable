// boolcmp.cc

#include <iostream>
#include <fstream>

// returns 0 for identical files and 1 for different files
int main(int argc, char* argv[])
{
    if (argc == 3)
    {
        std::fstream first(argv[1], std::ios::in | std::ios::binary);
        std::fstream second(argv[2], std::ios::in | std::ios::binary);

        bool identical = true;
        char currentFirst=0, currentSecond=0;
        while (first.good() && second.good())
        {
            first.read(&currentFirst, sizeof(currentFirst));
            second.read(&currentSecond, sizeof(currentSecond));
            if (currentFirst != currentSecond)
            {
                identical = false;
                break;
            }
        }
        if (first.good() || second.good())
        {
            identical = false;
        }
        if (identical == true)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        std::cout << "Usage: boolcmp <FirstFile> <SecondFile>\n\n";
        std::cout << "Returns error level 0 if files are identical, 1 otherwise.\n";
        return 1;
    }
}















