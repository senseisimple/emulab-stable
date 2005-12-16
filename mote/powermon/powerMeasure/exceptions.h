#include <stdexcept>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// Define classes to throw indicating errors
///////////////////////////////////////////////////////////////////////////////
class DataqException : public std::runtime_error {
public:

    DataqException(long int f_errorcode);
    virtual ~DataqException() throw();

    string what();
    long int errorcode;
    string msg;
};
