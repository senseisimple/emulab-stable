#include "testbed.h"
#include <stdlib.h>

testnode::testnode()
{
	name_ = NULL;
	//	interfaces_ = 0;
	type_ = 0;
	partition_ = 0;
}

testnode::~testnode()
{
	if (name_ != NULL)
		delete(name_);
}

void testnode::name(char *newname)
{
	if (name_ != NULL) {
		delete(name_);
	}
	name_ = new char[strlen(newname)+1];
	strcpy(name_, newname);
}

testedge::testedge()
{
	capacity_ = 0;
}

testedge::~testedge()
{
	/* Nothing yet */
}
