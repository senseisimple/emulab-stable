#ifndef UDP_THROUGHPUT_SENSOR_PELAB_H
#define UDP_THROUGHPUT_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"
#include "UdpPacketSensor.h"

class Sensor;
class UdpPacketSensor;

class UdpThroughputSensor:public Sensor{
	public:
		explicit UdpThroughputSensor( UdpPacketSensor * udpPacketSensorVal);
		~UdpThroughputSensor();
		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

	private:

		unsigned long long lastAckTime;
		double throughputKbps;
		UdpPacketSensor *packetHistory; 
};

#endif
