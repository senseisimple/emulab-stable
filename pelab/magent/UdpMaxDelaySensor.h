#ifndef UDP_MAX_DELAY_SENSOR_PELAB_H
#define UDP_MAX_DELAY_SENSOR_PELAB_H

#include "lib.h"
#include "UdpPacketSensor.h"
#include "UdpMinDelaySensor.h"
#include "Sensor.h"

class Sensor;
class UdpPacketSensor;
class UdpMinDelaySensor;

class UdpMaxDelaySensor:public Sensor{
	public:

		explicit UdpMaxDelaySensor(UdpPacketSensor *udpPacketSensorVal, UdpMinDelaySensor *minDelaySensorVal);
		~UdpMaxDelaySensor();


		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

	private:
		unsigned long long maxDelay;
		UdpPacketSensor *packetHistory;
		UdpMinDelaySensor *minDelaySensor;
};

#endif
