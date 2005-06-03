// trivial-ipassign.cc

#include <iostream>
#include <string>

using namespace std;

int main()
{
    string line;
    getline(cin, line);
    while (cin && line != "%%")
    {
        getline(cin, line);
    }
    int count = 0;
    getline(cin, line);
    while (cin)
    {
        cout << count << endl;
        getline(cin, line);
        ++count;
    }
    return 0;
}
