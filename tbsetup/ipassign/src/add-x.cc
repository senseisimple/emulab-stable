// add-x.cc

#include <iostream>

using namespace std;

int main()
{
    double current = 0.0;
    double total = 0.0;
    cin >> current;
    while (cin)
    {
        cout << total << ' ' << current << endl;
        total += 0.01;
        cin >> current;
    }
    return 0;
}
