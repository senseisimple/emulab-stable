// Decayer.cc

#include "lib.h"
#include "Decayer.h"

using namespace std;

Decayer::Decayer(int newValue, double newRate)
    : original(newValue)
    , decayed(newValue)
    , decayRate(newRate)
{
}

void Decayer::reset(int newValue)
{
    original = newValue;
    decayed = newValue;
}

int Decayer::get(void)
{
    return original;
}

void Decayer::decay(void)
{
    decayed = decayed - decayed*decayRate;
}

bool Decayer::operator<(int right) const
{
    return static_cast<int>(decayed) < right;
}

bool Decayer::operator<=(int right) const
{
    return static_cast<int>(decayed) <= right;
}

bool Decayer::operator>(int right) const
{
    return static_cast<int>(decayed) > right;
}

bool Decayer::operator>=(int right) const
{
    return static_cast<int>(decayed) >= right;
}

bool Decayer::operator==(int right) const
{
    return static_cast<int>(decayed) == right;
}

bool Decayer::operator!=(int right) const
{
    return static_cast<int>(decayed) != right;
}

bool operator<(int left, Decayer const & right)
{
    return right >= left;
}

bool operator<=(int left, Decayer const & right)
{
    return right > left;
}

bool operator>(int left, Decayer const & right)
{
    return right <= left;
}

bool operator>=(int left, Decayer const & right)
{
    return right < left;
}

bool operator==(int left, Decayer const & right)
{
    return right == left;
}

bool operator!=(int left, Decayer const & right)
{
    return right != left;
}
