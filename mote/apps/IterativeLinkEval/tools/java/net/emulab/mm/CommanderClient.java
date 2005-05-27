package net.emulab.mm;

import net.tinyos.message.*;

import net.emulab.packet.PktCmd;
import net.emulab.packet.PktRadioRecv;

public interface CommanderClient {

    //public void moteMessage(Message m,int moteID);
    
    public void cmdResponse(PktCmd m,int moteID);
    public void radioRecv(PktRadioRecv rr,int moteID);
    
}
