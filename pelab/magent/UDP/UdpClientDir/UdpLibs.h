#ifndef UDPLIBS_PELAB_H
#define UDPLIBS_PELAB_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <climits>
#include <limits.h>

namespace globalConsts {

	const short int USHORT_INT_SIZE = sizeof(unsigned short int);
	const short int ULONG_LONG_SIZE = sizeof(unsigned long long);
	const short int UCHAR_SIZE = sizeof(unsigned char);

	const static int redunAckSize = 2*USHORT_INT_SIZE + ULONG_LONG_SIZE;
	const static int seqNumSize = USHORT_INT_SIZE;
	const static int minAckPacketSize = 1 + 2*USHORT_INT_SIZE + ULONG_LONG_SIZE;
}

#endif
