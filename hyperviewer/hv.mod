%module hv
%{
//
// EMULAB-COPYRIGHT
// Copyright (c) 2004 University of Utah and the Flux Group.
// All rights reserved.
//
// Permission to use, copy, modify and distribute this software is hereby
// granted provided that (1) source code retains these copyright, permission,
// and disclaimer notices, and (2) redistributions including binaries
// reproduce the notices in supporting documentation.
//
// THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
// CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
// FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
//

#include <string>
NAMESPACEHACK

#include "HypView.h"
%}

/* Magic from /usr/local/share/doc/swig/Doc/Manual/Python.html
 * "19.8.2 Expanding a Python object into multiple arguments".
 */
%typemap(in) (int argc, char *argv[]) {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int i;
    $1 = PyList_Size($input);
    $2 = (char **) malloc(($1/*size*/+1)*sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
	$2[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
	PyErr_SetString(PyExc_TypeError,"list must contain strings");
	free($2);
	return NULL;
      }
    }
    $2[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}
%typemap(freearg) (int argc, char **argv) {
  free((char *) $2);
}

// It's easier to return the pointer to the HypView object rather than access the global.
extern HypView  *hvmain(int argc, char *argv[], int window, int width, int height);  
//extern int hvmain(int argc, char *argv[]);
//%include "cpointer.i"
//%pointer_class(HypView,hvp)
//extern HypView *hv;

// Separate out file reading from the main program.
extern int hvReadFile(char *fname, int width, int height);

// Get the node id string last selected by the selectCB function.
extern char *getSelected();

// std::string is used for INPUT args to HypView methods.
%include "std_string.i"
// How come %apply doesn't work for std::string?  Workaround with sed instead.
///namespace std {
//%apply const std::string & INPUT { const string & id };
///}

//================================================================

