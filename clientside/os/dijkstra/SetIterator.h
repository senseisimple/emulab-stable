// SetIterator.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef SET_ITERATOR_H_DISTRIBUTED_DIJKSTRA_1
#define SET_ITERATOR_H_DISTRIBUTED_DIJKSTRA_1

#include <cstring>

template<class T>
class DefaultCompare
{
public:
    int operator()(T const & left, T const & right)
    {
        if (left < right)
        {
            return -1;
        }
        else if (right < left)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
};

template<>
class DefaultCompare<std::string>
{
public:
    int operator()(std::string const & left, std::string const & right)
    {
        return left.compare(right);
    }
};

template<>
class DefaultCompare<char const *>
{
public:
    int operator()(char const * const & left, char const * const & right)
    {
        return strcmp(left, right);
    }
};

template<>
class DefaultCompare<char *>
{
public:
    int operator()(char * const & left, char * const & right)
    {
        return strcmp(left, right);
    }
};


//template<class T, class Compare/* = DefaultCompare<T::value>*/ >
class SetIterator
{
private:
    typedef std::set<int> T;
    typedef T::value_type value_type;
    typedef T::const_iterator iterator;
    typedef DefaultCompare<value_type> Compare;
public:
    enum SET
    {
        FIRST_SET,
        SECOND_SET,
        BOTH_SETS
    };
public:
    SetIterator(T const & first, T const & second)
    {
        reset(first, second);
    }

    SetIterator(T const & first, T const & second,
                Compare const & newPredicate)
    {
        reset(first, second, newPredicate);
    }

    ~SetIterator()
    {
    }

    SetIterator(SetIterator const & right)
        : firstPos(right.firstPos)
        , firstEnd(right.firstEnd)
        , secondPos(right.secondPos)
        , secondEnd(right.secondEnd)
        , predicate(right.predicate)
        , currentSet(right.currentSet)
    {
    }

    SetIterator & operator=(SetIterator const & right)
    {
        firstPos = right.firstPos;
        firstEnd = right.firstEnd;
        secondPos = right.secondPos;
        secondEnd = right.secondEnd;
        predicate = right.predicate;
        currentSet = right.currentSet;
        return *this;
    }

    void reset(T const & first, T const & second)
    {
        reset(first, second, Compare());
    }

    void reset(T const & first, T const & second, Compare const & newPredicate)
    {
        predicate = newPredicate;
        firstPos = first.begin();
        firstEnd = first.end();
        secondPos = second.begin();
        secondEnd = second.end();
        setCurrentSet();
    }

    bool valid(void)
    {
        return !(firstPos == firstEnd && secondPos == secondEnd);
    }

    value_type const & getValue(void)
    {
        assert(valid());
        switch (currentSet)
        {
        case FIRST_SET:
            return *firstPos;
        case SECOND_SET:
        case BOTH_SETS:
        default:
            return *secondPos;
        }
    }

    SET getSet(void)
    {
        return currentSet;
    }

    void increment(void)
    {
        assert(valid());
        if (firstPos == firstEnd)
        {
            ++secondPos;
        }
        else if (secondPos == secondEnd)
        {
            ++firstPos;
        }
        else
        {
            switch (currentSet)
            {
            case FIRST_SET:
                ++firstPos;
                break;
            case SECOND_SET:
                ++secondPos;
                break;
            case BOTH_SETS:
                ++firstPos;
                ++secondPos;
                break;
            default:
                break;
            }
        }
        setCurrentSet();
    }
private:
    void setCurrentSet(void)
    {
        if (valid())
        {
            if (firstPos == firstEnd)
            {
                currentSet = SECOND_SET;
            }
            else if (secondPos == secondEnd)
            {
                currentSet = FIRST_SET;
            }
            else
            {
                int comparison = predicate(*firstPos, *secondPos);
                if (comparison == 0)
                {
                    currentSet = BOTH_SETS;
                }
                else if (comparison < 0)
                {
                    currentSet = FIRST_SET;
                }
                else
                {
                    currentSet = SECOND_SET;
                }
            }
        }
    }
private:
    SetIterator();
private:
    iterator firstPos;
    iterator firstEnd;
    iterator secondPos;
    iterator secondEnd;
    Compare predicate;
    SET currentSet;
};

#endif
