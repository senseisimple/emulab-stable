#ifndef UDP_LOSS_SENSOR_PELAB_H
#define UDP_LOSS_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"
#include "UdpPacketSensor.h"
#include "UdpRttSensor.h"

class UdpSensor;
class UdpPacketSensor;
class UdpRttSensor;

class UdpLossSensor:public UdpSensor{
	public:

		explicit UdpLossSensor(UdpPacketSensor *packetHistoryVal, UdpRttSensor *rttSensorVal, UdpState &udpStateVal, ofstream &logStreamVal);
		~UdpLossSensor();


		void localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);
		void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);

		long getPacketLoss();
		void resetLoss();
		long totalLoss;
	private:
		long packetLoss;
		UdpState &udpStateInfo;
		ofstream &logStream;

		UdpPacketSensor *packetHistory;
		UdpRttSensor *rttSensor;
};

#endif
