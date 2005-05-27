package net.emulab.mm;

import net.tinyos.message.Message;
import net.tinyos.message.MessageListener;
import net.tinyos.message.MoteIF;

import net.emulab.packet.PktCmd;
import net.emulab.packet.PktRadioRecv;

public class CommanderMessageListener implements MessageListener {
    
    private CommanderClient cc;
    private MoteIF myIF;
    private int moteID;

    public CommanderMessageListener(CommanderClient cc,
				    MoteIF myIF,
				    int moteID) {
	this.cc = cc;
	this.myIF = myIF;
	this.moteID = moteID;
    }

    public void messageReceived(int to, Message m) {
	if (m != null) {
	    if (m instanceof PktCmd) {
		cc.cmdResponse((PktCmd)m,moteID);
	    }
	    else if (m instanceof PktRadioRecv) {
		cc.radioRecv((PktRadioRecv)m,moteID);
	    }
	    else {
		;
	    }
	}
    }
}
