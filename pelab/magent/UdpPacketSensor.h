/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
    //CHANGE:
    std::list<UdpPacketInfo>& getUnAckedPacketList();


  private:

    //CHANGE:
    bool handleReorderedAck(PacketInfo *packet);

    std::list<UdpPacketInfo> sentPacketList;
    std::vector< UdpPacketInfo > ackedPackets;
    //CHANGE:
    std::list<UdpPacketInfo> unAckedPacketList;

    long lastSeenSeqNum;
    int packetLoss;
    int totalPacketLoss;

    int libpcapSendLoss;
    //CHANGE:
    int statReorderedPackets;
    bool ackFake;


};


#endif
