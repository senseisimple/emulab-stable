/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
