// Exception.h

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

