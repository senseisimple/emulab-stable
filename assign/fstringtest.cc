/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Test program for the fstring library.
 */

#include "fstring.h"
#include <iostream>
using namespace std;
#include <ext/hash_map>
using namespace __gnu_cxx;
#include <map>
using namespace std;

typedef map<fstring,int> stringmap;
typedef hash_map<fstring,int> stringhmap;

int main() {
	fstring string1("Hybrid Rainbow");
	cout << "string1 is " << string1 << ", hashes to " << string1.hash() << endl;
    cout << "unique strings: " << fstring::count_unique_strings() << endl;
	fstring string2("Blues Drive Monster");
	cout << "string2 is " << string2 << ", hashes to " << string2.hash() << endl;
    cout << "unique strings: " << fstring::count_unique_strings() << endl;
	fstring string3("Hybrid Rainbow");
	cout << "string3 is " << string3 << ", hashes to " << string3.hash() << endl;
    cout << "unique strings: " << fstring::count_unique_strings() << endl;

    cout << "Does string1 == string3? ";
    if (string1 == string3) {
        cout << "yes";
    } else {
        cout << "no";
    }
    cout << endl;

    cout << "Does string1 == string2? ";
    if (string1 == string2) {
        cout << "yes";
    } else {
        cout << "no";
    }
    cout << endl;

    cout << endl << "map test" << endl;
    stringmap map;
    cout << "map[string1] = 1" << endl;
    map[string1] = 1;
    cout << "map[string2] = 2" << endl;
    map[string2] = 2;
    cout << "Testing: map[string1] = " << map[string1] << endl;
    cout << "Testing: map[string2] = " << map[string2] << endl;
    cout << "Testing: map[string3] = " << map[string3] << endl;

    cout << endl << "hash_map test" << endl;
    stringhmap hmap;
    cout << "hmap[string1] = 1" << endl;
    hmap[string1] = 1;
    cout << "hmap[string2] = 2" << endl;
    hmap[string2] = 2;
    cout << "Testing: hmap[string1] = " << hmap[string1] << endl;
    cout << "Testing: hmap[string2] = " << hmap[string2] << endl;
    cout << "Testing: hmap[string3] = " << hmap[string3] << endl;
	
}
