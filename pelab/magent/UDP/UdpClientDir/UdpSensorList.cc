#include "UdpSensorList.h"

UdpSensorList::UdpSensorList(ofstream &logStreamVal)
	:logStream(logStreamVal),
	sensorListHead(NULL),
	sensorListTail(NULL),
	depPacketSensor(NULL),
	depThroughputSensor(NULL),
	depMinDelaySensor(NULL),
	depMaxDelaySensor(NULL),
	depRttSensor(NULL),
	depLossSensor(NULL),
	depAvgThroughputSensor(NULL)
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
	delete depRttSensor;
	delete depLossSensor;
	delete depAvgThroughputSensor;
}

void UdpSensorList::capturePacket(char *packetData, int Len, int overheadLen, unsigned long long timeStamp, int packetDirection)
{
	UdpSensor *listIter = sensorListHead;

	while(listIter != NULL)
	{
		listIter->capturePacket(packetData, Len, overheadLen, timeStamp, packetDirection);
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
		case UDP_RTT_SENSOR:
			addRttSensor();
			break;
		case UDP_LOSS_SENSOR:
			addLossSensor();
			break;
		case UDP_AVG_THROUGHPUT_SENSOR:
			addAvgThroughputSensor();

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
		//printf("Adding packet sensor\n");
		depPacketSensor = new UdpPacketSensor(udpStateInfo, logStream);

		pushSensor(depPacketSensor);
	}
}

void UdpSensorList::addThroughputSensor()
{
	addPacketSensor();

	if(depThroughputSensor == NULL)
	{
		//printf("Adding throughput sensor\n");
		depThroughputSensor = new UdpThroughputSensor(udpStateInfo, logStream);

		pushSensor(depThroughputSensor);
	}

}
void UdpSensorList::addMinDelaySensor()
{
	addPacketSensor();

	if(depMinDelaySensor == NULL)
	{
		//printf("Adding mindelay sensor\n");
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
		//printf("Adding maxdelay sensor\n");
		depMaxDelaySensor = new UdpMaxDelaySensor(udpStateInfo, logStream);

		pushSensor(depMaxDelaySensor);
	}
}

void UdpSensorList::addRttSensor()
{
	addPacketSensor();

	if(depRttSensor == NULL)
	{
		//printf("Adding rtt sensor\n");
		depRttSensor = new UdpRttSensor(udpStateInfo, logStream);

		pushSensor(depRttSensor);
	}


}

void UdpSensorList::addLossSensor()
{
	addPacketSensor();
	addRttSensor();

	if(depLossSensor == NULL)
	{
		//printf("Adding loss sensor\n");
		depLossSensor = new UdpLossSensor(depPacketSensor, depRttSensor,udpStateInfo, logStream);

		pushSensor(depLossSensor);

	}

}

void UdpSensorList::addAvgThroughputSensor()
{
	addLossSensor();

	if(depAvgThroughputSensor == NULL)
	{
		//printf("Adding avgThroughput sensor\n");
		depAvgThroughputSensor = new UdpAvgThroughputSensor(depLossSensor, udpStateInfo, logStream);
		pushSensor(depAvgThroughputSensor);
	}

}
