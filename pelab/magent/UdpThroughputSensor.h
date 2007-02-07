/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef UDP_THROUGHPUT_SENSOR_PELAB_H
#define UDP_THROUGHPUT_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"
#include "UdpPacketSensor.h"

class Sensor;
class UdpPacketSensor;

class UdpThroughputSensor:public Sensor{
	public:
		explicit UdpThroughputSensor( UdpPacketSensor const * udpPacketSensorVal);
		~UdpThroughputSensor();
		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

	private:

		unsigned long long lastAckTime;
		double throughputKbps;
		UdpPacketSensor const *packetHistory; 
};

#endif
