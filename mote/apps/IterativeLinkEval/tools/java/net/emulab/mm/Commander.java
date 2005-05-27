package net.emulab.mm;

import net.emulab.packet.PktCmd;
import net.emulab.packet.PktRadioRecv;

import net.tinyos.message.Message;

import java.util.Vector;
import java.io.IOException;

public interface Commander {

    public static int ERROR = 1;
    public static int SUCCESS = 0;

    public static short CMD_TYPE_BCAST = 0;
    public static short CMD_TYPE_BCAST_MULT = 1;
    public static short CMD_TYPE_RADIO_OFF = 2;
    public static short CMD_TYPE_RADIO_ON = 3;
    public static short CMD_TYPE_SET_MOTE_ID = 4;
    public static short CMD_TYPE_SET_RF_POWER = 5;
    public static short CMD_TYPE_HEARTBEAT = 6;

    public static short CMD_STATUS_SUCCESS = 0;
    public static short CMD_STATUS_ERROR = 1;
    public static short CMD_STATUS_UNKNOWN = 2;

    public static short CMD_FLAGS_INCR_FIRST = 0x1;
    


    public int sendCmd(PktCmd m, int moteID) throws IOException;
    public int sendMessage(Message m, int moteID) throws IOException;

    public Vector getMoteList();

    public int removeMote(int moteID);
    public int addMote(String spec);
    public int addMote(String host,int port);

    public int disconnect();
    public int disconnect(int moteID);
    public int connect();
    public int connect(int moteID);

}
