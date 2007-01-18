#ifndef UDP_PACKET_SENSOR_PELAB_H
#define UDP_PACKET_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"

using namespace std;

class UdpPacketInfo;
class equalSeqNum;
class Sensor;


class UdpPacketSensor:public Sensor{

  public:

    UdpPacketSensor();
    ~UdpPacketSensor();
    void localSend(PacketInfo *packet);
    void localAck(PacketInfo *packet);
    bool isAckFake();
    int getPacketLoss();

    vector< UdpPacketInfo > ackedPackets;

  private:

    list<UdpPacketInfo> sentPacketList;

    long lastSeenSeqNum;
    int packetLoss;
    int totalPacketLoss;

    int libpcapSendLoss;
    bool ackFake;

};


#endif
