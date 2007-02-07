/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef UDP_MAX_DELAY_SENSOR_PELAB_H
#define UDP_MAX_DELAY_SENSOR_PELAB_H

#include "lib.h"
#include "UdpPacketSensor.h"
#include "UdpMinDelaySensor.h"
#include "Sensor.h"

class Sensor;
class UdpPacketSensor;
class UdpMinDelaySensor;
struct UdpPacketInfo;

class UdpMaxDelaySensor:public Sensor{
	public:

		explicit UdpMaxDelaySensor(UdpPacketSensor const *udpPacketSensorVal, UdpMinDelaySensor const *minDelaySensorVal);
		~UdpMaxDelaySensor();


		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

	private:
		unsigned long long maxDelay;
		UdpPacketSensor const *packetHistory;
		UdpMinDelaySensor const *minDelaySensor;
};

#endif
