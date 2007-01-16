#include "UdpSensor.h"

using namespace std;

UdpSensor::UdpSensor()
{

}

UdpSensor::~UdpSensor()
{

}

void UdpSensor::capturePacket(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	if(Len < 1)
	{
		cout << "Error: UDP packet data sent to Sensor was less than "
			" 1 byte in size\n";
		return;
	}

	if(packetData[0] == '0')
	{
		localSend( (packetData+1), Len - 1,overheadLen, timeStamp );
	}
	else if(packetData[0] == '1')
	{
		localAck( (packetData + 1), Len - 1,overheadLen, timeStamp );
	}

}
