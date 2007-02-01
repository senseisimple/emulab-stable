#ifndef UDP_LOSS_SENSOR_PELAB_H
#define UDP_LOSS_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"
#include "UdpPacketSensor.h"
#include "UdpRttSensor.h"

class Sensor;
class UdpPacketSensor;
class UdpRttSensor;

class UdpLossSensor:public Sensor{
	public:

		explicit UdpLossSensor(UdpPacketSensor const *packetHistoryVal, UdpRttSensor const *rttSensorVal);
		~UdpLossSensor();


		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

		long getPacketLoss() const;
		long getTotalPacketLoss() const;
		void resetLoss();

	private:
		long packetLoss;
		long totalLoss;

		UdpPacketSensor const *packetHistory;
		UdpRttSensor const *rttSensor;
};

#endif
