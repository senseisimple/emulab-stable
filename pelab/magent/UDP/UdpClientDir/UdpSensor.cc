/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpSensor.h"

using namespace std;

UdpSensor::UdpSensor()
{

}

UdpSensor::~UdpSensor()
{

}

void UdpSensor::capturePacket(char *packetData, int Len, int overheadLen, unsigned long long timeStamp, int packetDirection)
{
	if(Len < 1)
	{
		cout << "Error: UDP packet data sent to Sensor was less than "
			" 1 byte in size\n";
		return;
	}

	if(packetDirection == 0)
	{
		localSend( (packetData + 1), Len - 1,overheadLen, timeStamp );
	}
	else if(packetDirection == 1)
	{
		localAck( (packetData), Len ,overheadLen, timeStamp );
	}

}
