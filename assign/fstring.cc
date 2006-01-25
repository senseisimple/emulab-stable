#include "fstring.h"

fstring::stringmap fstring::all_strings;
const char *fstring::emptystr = "";

const char *fstring::unique_string(const char *str) {
    // First, check to see if this string is already in the map
    stringmap::const_iterator it = all_strings.find(str);
    if (it == all_strings.end()) {
        // Not in there, insert it and return the new string
        char *stored_str = (char*)malloc(strlen(str) + 1);
        strcpy(stored_str,str);
        all_strings[stored_str] = stored_str;
        return stored_str;
    } else {
        return it->second;
    }
}

/*
 * Note: Many functions are in the header file, becuase I want them
 * inlined
 */
