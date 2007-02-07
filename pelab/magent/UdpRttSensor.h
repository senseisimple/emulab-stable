/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef UDP_RTT_SENSOR_PELAB_H
#define UDP_RTT_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"
#include "UdpPacketSensor.h"

class UdpPacketInfo;
class equalSeqNum;
class Sensor;
class UdpPacketSensor;

class UdpRttSensor:public Sensor{
	public:

		explicit UdpRttSensor(UdpPacketSensor const *packetHistoryVal);
		~UdpRttSensor();


		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);
		unsigned long long getRtt() const;
		unsigned long long getDevRtt() const;

	private:
		unsigned long long ewmaRtt, ewmaDevRtt;
		UdpPacketSensor const *packetHistory;
};

#endif
