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
    virtual const char* what() const throw()
    {
        return message.c_str();
    }
private:
    string message;
};

class BitOverflowError : public StringException
{
public:
    explicit BitOverflowError(std::string const & error)
        : StringException(error)
    {
    }
};

class InvalidCharacterException : public StringException
{
public:
    explicit InvalidCharacterException(std::string const & error)
        : StringException(error)
    {
    }
};

class EmptyLineException : public StringException
{
public:
    explicit EmptyLineException(std::string const & error)
        : StringException(error)
    {
    }
};

class NotEnoughNodesException : public StringException
{
public:
    explicit NotEnoughNodesException(std::string const & error)
        : StringException(error)
    {
    }
};



#endif

