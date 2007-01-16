#include "UdpSensorList.h"

UdpSensorList::UdpSensorList(ofstream &logStreamVal)
	:logStream(logStreamVal),
	sensorListHead(NULL),
	sensorListTail(NULL),
	depPacketSensor(NULL),
	depThroughputSensor(NULL),
	depMinDelaySensor(NULL),
	depMaxDelaySensor(NULL)
{
}

UdpSensorList::~UdpSensorList()
{
	// Delete the list of sensors.
	if(sensorListHead != NULL)
	{
		UdpSensor *tempPtr;

		while(sensorListHead != NULL)
		{
			tempPtr = sensorListHead;
			sensorListHead = sensorListHead->next;
			delete tempPtr;
		}

	}

	delete depPacketSensor;
	delete depThroughputSensor;
	delete depMinDelaySensor;
	delete depMaxDelaySensor;
}

void UdpSensorList::capturePacket(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	UdpSensor *listIter = sensorListHead;

	while(listIter != NULL)
	{
		listIter->capturePacket(packetData, Len, overheadLen, timeStamp);
		listIter = listIter->next;
	}

}

void UdpSensorList::addSensor(int sensorName)
{
	switch(sensorName)
	{
		case UDP_PACKET_SENSOR:
			addPacketSensor();
			break;

		case UDP_THROUGHPUT_SENSOR:
			addThroughputSensor();
			break;

		case UDP_MINDELAY_SENSOR:
			addMinDelaySensor();
			break;

		case UDP_MAXDELAY_SENSOR:
			addMaxDelaySensor();
			break;

		default:
			break;
	}
}

void UdpSensorList::pushSensor(UdpSensor *sensorPtr)
{
	if(sensorListHead == NULL)
	{
		sensorListHead = sensorPtr;
		sensorListTail = sensorListHead;

		sensorListTail->next = NULL;
	}
	else
	{
		sensorListTail->next = sensorPtr;

		sensorListTail = sensorListTail->next;
		sensorListTail->next = NULL;
	}
}

void UdpSensorList::reset()
{
	// Delete the list of sensors.
	if(sensorListHead != NULL)
	{
		UdpSensor *tempPtr;

		while(sensorListHead != NULL)
		{
			tempPtr = sensorListHead;
			sensorListHead = sensorListHead->next;
			delete tempPtr;
		}

	}
	// Delete our local pointers too - not necessary, just to be on the safe side.
	delete depPacketSensor;
	delete depThroughputSensor;
	delete depMinDelaySensor;
	delete depMaxDelaySensor;

	udpStateInfo.reset();

	sensorListHead = NULL;
	sensorListTail = NULL;

	depPacketSensor = NULL;
	depThroughputSensor = NULL;
	depMinDelaySensor = NULL;
	depMaxDelaySensor = NULL;
}

void UdpSensorList::addPacketSensor()
{
	if(depPacketSensor == NULL)
	{
		depPacketSensor = new UdpPacketSensor(udpStateInfo, logStream);

		pushSensor(depPacketSensor);
	}
}

void UdpSensorList::addThroughputSensor()
{
	addPacketSensor();

	if(depThroughputSensor == NULL)
	{
		depThroughputSensor = new UdpThroughputSensor(udpStateInfo, logStream);

		pushSensor(depThroughputSensor);
	}

}
void UdpSensorList::addMinDelaySensor()
{
	addPacketSensor();

	if(depMinDelaySensor == NULL)
	{
		depMinDelaySensor = new UdpMinDelaySensor(udpStateInfo, logStream);

		pushSensor(depMinDelaySensor);

	}

}
void UdpSensorList::addMaxDelaySensor()
{
	addPacketSensor();
	addMinDelaySensor();

	if(depMaxDelaySensor == NULL)
	{
		depMaxDelaySensor = new UdpMaxDelaySensor(udpStateInfo, logStream);

		pushSensor(depMaxDelaySensor);
	}
}
