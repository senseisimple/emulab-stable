#ifndef UDP_PACKET_SENSOR_PELAB_H
#define UDP_PACKET_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"

class UdpPacketInfo;
class equalSeqNum;
class Sensor;


class UdpPacketSensor:public Sensor{

  public:

    UdpPacketSensor();
    ~UdpPacketSensor();
    void localSend(PacketInfo *packet);
    void localAck(PacketInfo *packet);
    bool isAckFake() const;
    int getPacketLoss() const;
    std::vector<UdpPacketInfo> getAckedPackets() const;


  private:

    std::list<UdpPacketInfo> sentPacketList;
    std::vector< UdpPacketInfo > ackedPackets;

    long lastSeenSeqNum;
    int packetLoss;
    int totalPacketLoss;

    int libpcapSendLoss;
    bool ackFake;

};


#endif
