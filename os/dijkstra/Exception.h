// Exception.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Exception.h defines all of the possible things that might go wrong with
// the program. Each one has a string associated with it that is printed
// to the user as output.

#ifndef EXCEPTION_H_IP_ASSIGN_1
#define EXCEPTION_H_IP_ASSIGN_1

#include <exception>

class StringException : public std::exception
{
public:
    explicit StringException(std::string const & error)
        : message(error)
    {
    }
    virtual ~StringException() throw() {}
    virtual char const * what() const throw()
    {
        return message.c_str();
    }
    virtual void addToMessage(char const * addend)
    {
        message += addend;
    }
    virtual void addToMessage(std::string const & addend)
    {
        addToMessage(addend.c_str());
    }
private:
    std::string message;
};

class InvalidArgumentException : public StringException
{
public:
    explicit InvalidArgumentException(std::string const & error)
        : StringException("Invalid Argument: " + error)
    {
    }
    virtual ~InvalidArgumentException() throw() {}
};

#endif

