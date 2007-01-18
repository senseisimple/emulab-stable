#ifndef UDP_MIN_DELAY_SENSOR_PELAB_H
#define UDP_MIN_DELAY_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"
#include "UdpPacketSensor.h"

class Sensor;
class UdpPacketSensor;

class UdpMinDelaySensor:public Sensor{
	public:

		explicit UdpMinDelaySensor(UdpPacketSensor *udpPacketSensorVal);
		~UdpMinDelaySensor();

		unsigned long long getMinDelay();
		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

	private:
		unsigned long long minDelay;
		UdpPacketSensor *packetHistory;
};

#endif
