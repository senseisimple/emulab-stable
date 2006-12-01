/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <string>

using namespace std;

#include "exceptions.h"
#include "dataqsdk.h"
#include "constants.h"


///////////////////////////////////////////////////////////////////////////////
// Implement classes to throw indicating errors
///////////////////////////////////////////////////////////////////////////////

DataqException::DataqException( long int f_errorcode )
: std::runtime_error("DataqException")
{
    errorcode = f_errorcode;
}


DataqException::~DataqException() throw()
{
}

string DataqException::what()
{
    if(		  errorcode == ENODEV ){
        msg = "DataqException: No Device Specified";
    }else if( errorcode == ENOSYS ){
        msg = "DataqException: Function not supported";
    }else if( errorcode == EBUSY ){
        msg = "DataqException: Acquiring/Busy";
    }else if( errorcode == ENOLINK  ){
        msg = "DataqException: Not Connected";
    }else if( errorcode == EINVAL   ){
        msg = "DataqException: Bad parameter pointer";
    }else if( errorcode == EBOUNDS  ){
        msg = "DataqException: Parameter value(s) out of bounds.";
    }else{
        msg = "DataqException: unknown";
    }
    return msg;
}
