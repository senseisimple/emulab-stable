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
	capacity_ = 100;  /* XXX:  Jay would like us to eventually change
			   * this based upon feedback, etc.
			   * We also should deal specially with < 10Mb
			   * links so we can ifconfig the interface
			   * to 10Mb and need worry less about congestion
			   */
}

testedge::~testedge()
{
	/* Nothing yet */
}
