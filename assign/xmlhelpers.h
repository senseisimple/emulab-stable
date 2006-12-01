/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __XMLHELPERS_H
#define __XMLHELPERS_H

/*
 * xmlhelpers.h - Classes and functions to make XML parsing a little easier
 */

#include <string>
using namespace std;
#include <xercesc/util/XMLString.hpp>
XERCES_CPP_NAMESPACE_USE

/*
 * This class provides for conversion between Xerces' internal XMLCh* type
 * (which is 16 bits wide to hold international characters) and simple
 * char*s. You can construct an XStr with either type, and then call C() or
 * x() to get back the desired type of string. XStr works out on its own what
 * conversion needs to happen, and frees it up in its destructor (normally,
 * when it goes out of scope.)
 *
 * DO NOT MAKE POINTERS TO THIS CLASS - that would defeat the whole purpose
 * of it's string memory management.
 */
class XStr {
    public:
        XStr(char *_str) : cstr(_str), cstr_mine(false), xmlstr(NULL),
                           xmlstr_mine(false) { ; };
        XStr(const XMLCh *_str) : cstr(NULL), cstr_mine(false), xmlstr(NULL),
                           xmlstr_mine(true) {
            xmlstr = XMLString::replicate(_str);
        };

        ~XStr() {
            if (cstr_mine && cstr != NULL) {
                XMLString::release(&cstr);
            }
            if (xmlstr_mine && xmlstr != NULL) {
                XMLString::release(&xmlstr);
            }
        };

	/*
	 * Convert to a Xerces string (XMLCh *)
	 */
        const XMLCh *x() {
            if (this->xmlstr == NULL) {
                this->xmlstr = XMLString::transcode(this->cstr);
                this->xmlstr_mine = true;
            }
            return this->xmlstr;
        };

	/*
	 * convert to a C-style string (null-terminated char*)
	 */
        char *c() {
            if (this->cstr == NULL) {
                this->cstr = XMLString::transcode(this->xmlstr);
                this->cstr_mine = true;
            }
            return this->cstr;
        };

        bool operator==(const char * const other) {
            return (strcmp(this->c(),other) == 0);
        };
        bool operator!=(const char * const other) {
            return (strcmp(this->c(),other) != 0);
        };
	
	/*
	 * Crazy, I know, but this is how you tell C++ it can implicilty convert an
	 * XString into other data types. Thanks to Jon Duerig and the ghost of Bjarne
	 * Stroustrup (really, his C++ book) for helping me figure this out.
	 */
	operator char*() { return this->c(); };
	// Note, this operator commented out because some functions that can take
	// either a char* or an XMLch* get confused if we have conversions to both
	//operator const XMLCh*() { return this->x(); };

    private:
        char *cstr;
        bool cstr_mine;
        XMLCh *xmlstr;
        bool xmlstr_mine;
};

#endif
