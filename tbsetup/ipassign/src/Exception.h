// Exception.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Exception.h defines all of the possible things that might go wrong with
// the program. Each one has a string associated with it that is printed
// to the user as output.

// Note that as of now, the strings are completely determined by the
// creator of an Exception object. I ought to go back and change this
// so that each Exception contributes its exception name to the beginning
// of the string and the Exception creator just provides the incidental
// information.

#ifndef EXCEPTION_H_IP_ASSIGN_1
#define EXCEPTION_H_IP_ASSIGN_1

class StringException : public std::exception
{
public:
    explicit StringException(std::string const & error)
        : message(error)
    {
    }
    virtual char const * what() const throw()
    {
        return message.c_str();
    }
    virtual void addToMessage(char const * addend)
    {
        message += addend;
    }
    virtual void addToMessage(string const & addend)
    {
        addToMessage(addend.c_str());
    }
private:
    string message;
};

class BitOverflowException : public StringException
{
public:
    explicit BitOverflowException(std::string const & error)
        : StringException("Too many hosts and lans: " + error)
    {
    }
};

class InvalidCharacterException : public StringException
{
public:
    explicit InvalidCharacterException(std::string const & error)
        : StringException("Invalid character(s) in line: " + error)
    {
    }
};

class MissingWeightException : public StringException
{
public:
    explicit MissingWeightException(std::string const & error)
        : StringException("Missing weight in line: " + error)
    {
    }
};

class NotEnoughNodesException : public StringException
{
public:
    explicit NotEnoughNodesException(std::string const & error)
        : StringException("Not enough nodes in line: " + error)
    {
    }
};

class NoHeaderException : public StringException
{
public:
    explicit NoHeaderException(std::string const & error)
        : StringException("Error Reading Header: " + error)
    {
    }
};

class InvalidArgumentException : public StringException
{
public:
    explicit InvalidArgumentException(std::string const & error)
        : StringException("Invalid Argument: " + error)
    {
    }
};

class ImpossibleConditionException : public StringException
{
public:
    explicit ImpossibleConditionException(std::string const & error)
        : StringException("Impossible Condition in Function: " + error)
    {
    }
};

class NoConnectionException : public StringException
{
public:
    explicit NoConnectionException(std::string const & error)
        : StringException("Oops. I tried to get a connection"
                          " which doesn't exist: " + error)
    {
    }
};

class NoGraphToRouteException : public StringException
{
public:
    explicit NoGraphToRouteException(std::string const & error)
        : StringException("I was asked to set up routing without a graph: "
                          + error)
    {
    }
};

class NoTopLevelException : public StringException
{
public:
    explicit NoTopLevelException(std::string const & error)
        : StringException("Routing Graph had no top level: " + error)
    {
    }
};

#endif

