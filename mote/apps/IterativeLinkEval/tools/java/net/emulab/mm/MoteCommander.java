package net.emulab.mm;

import net.emulab.packet.*;
import net.tinyos.message.*;
import net.tinyos.message.MoteIF;
import net.tinyos.message.MessageListener;
import net.tinyos.packet.*;
import net.tinyos.packet.PhoenixSource;
import net.tinyos.packet.BuildSource;
import net.tinyos.util.*;

import java.text.ParseException;
import java.io.IOException;

import java.util.Vector;
import java.util.Hashtable;
import java.util.Enumeration;

public class MoteCommander implements 
			       net.emulab.mm.Commander,
			       net.tinyos.packet.PhoenixError
{

    /**
     * MoteManager is capable of speaking to motes that understand
     * the packet structure of packets in the net/emulab/packet/ dir.
     * Essentially, it can broadcast packets with 10 bytes of data, 
     * record any received packets of this kind, and accept certain kinds 
     * of commands:
     *   - set mote id
     *   - set RF power
     *     For the CC2420 radios:
     *       Set the transmit RF power value.
     *       The input value is simply an arbitrary
     *       index that is programmed into the CC2420 registers.
     *       The output power is set by programming the power amplifier.
     *       Valid values are 1 through 31 with power of 1 equal to
     *       -25dBm and 31 equal to max power (0dBm)
     *     For the CC1000 radios:
     *       Index between 1 and 255.
     *       from contrib/xbow/tools/src/xlisten/boards/mica2.c
     *       int table_916MHz_power[21][2] = {{2,-20},{4,-16},{5,-14},{6,-13},
     *                              {7,-12},{8,-11},{9,-10},{11,-9},{12,-8},
     *             {13,-7},{15,-6},{64,-5},{80,-4},{96,-2},{112,-1},{128,0},
     *             {144,1},{176,2},{192,3},{240,4},{255,5}}      ;

     short[] powerLevels = { 2,4,5,6,7,8,9,11,12,13,15,
                             64,80,96,112,128,144,176,192,240,255
				0x3,0x4,0x5,0x6,0x7,0x8,0x9,
				0xb,0xc,0xd,0xf,
				0x40,0x50,0x60,0x70,0x80,0x90,0xb0,0xc0,
				0xf0,
				0xff };

     *
     *       dBm] / hex
     *       -20  02
     *       -19  02
     *       -18  03
     *       -17  03
     *       -16  04
     *       -15  05
     *       -14  05
     *       -13  06
     *       -12  07
     *       -11  08
     *       -10  09
     *       -9   0b
     *       -8   0c
     *       -7   0d
     *       -6   0f
     *       -5   40
     *       -4   50
     *       -3   50
     *       -2   60
     *       -1   70
     *        0   80
     *        1   90
     *        2   b0
     *        3   c0
     *        4   f0
     *        5   ff
     *       Now, these values are for 868MHz, but the datasheet just did not
     *       have hex values for anything but 868 and 433 MHz.  Weird.
     *   - turn radio on/off
     *   - send a packet with src,dest motes and 10 bytes of data.
     *   - send multiples of these packets... mult_bcast mode.
     */

    public static String DEFAULT_SF_HOST = "localhost";
    public static int DEFAULT_SF_PORT = 9000;

    private Hashtable moteTable;
    private Hashtable moteMessageListeners;
    private CommanderClient cc;

    public MoteCommander() {
	this.moteTable = new Hashtable();
	this.moteMessageListeners = new Hashtable();
	cc = null;
    }

    public MoteCommander(CommanderClient cc,String motes) 
	throws ParseException,Exception {
	this.cc = cc;
	this.moteTable = new Hashtable();
	this.moteMessageListeners = new Hashtable();
	parseCreate(motes);
    }

    public MoteCommander(CommanderClient cc,
			 String host,
			 int basePort,
			 String motes) 
	throws ParseException,Exception {
	this.cc = cc;
	this.moteTable = new Hashtable();
	this.moteMessageListeners = new Hashtable();
	parseCreate(host,basePort,motes);
    }

    private void parseCreate(String hostname,int basePort,String moteIDs) 
	throws ParseException, Exception {
	String[] ids = moteIDs.split(",");
	for (int i = 0; i < ids.length; ++i) {

	    int port = -1;
	    int mid = -1;
	    try {
		port = Integer.parseInt(ids[i]);
		mid = Integer.parseInt(ids[i]);
	    }
	    catch (Exception e) {
		throw e;
	    }

	    MoteIF iface = null;
	    PhoenixSource phs = null;
	    try {
		phs = BuildSource.makePhoenix("sf@" + hostname + ":" + 
					      (basePort + port),
					      null);
		phs.setPacketErrorHandler(this);
		iface = new MoteIF(phs);
	    }
	    catch (Exception e) {
		throw e;
	    }

	    if (iface != null) {
		System.out.println("added phs for mote id " + mid);
		moteTable.put(new Integer(mid),iface);
		CommanderMessageListener l = 
		    new CommanderMessageListener(cc,iface,mid);
		iface.registerListener(new PktCmd(),l);
		iface.registerListener(new PktRadioRecv(),l);
		moteMessageListeners.put(new Integer(mid),l);
	    }
	}
    }

    // should be like:
    // "sfcontrol.emulab.net,9000,22;192.168.0.1,3500,23"
    private void parseCreate(String motes) throws ParseException,Exception {
	//nsString = motes.replaceAll(" ","");
	String[] splitMotes = motes.split(";");
	int currentIndex = 0;
	
	for (int i = 0; i < splitMotes.length; ++i) {
	    String[] singleMote = splitMotes[i].split(",");
	    String host;
	    int port;
	    int moteID;

	    if (singleMote.length != 3) {
		currentIndex += splitMotes[i].length();
		throw new ParseException("ERROR: bad mote specification: " +
					 "\"" + splitMotes[i] + "\"",
					 currentIndex);
		
	    }
	    else {
		host = singleMote[0].replaceAll(" ","");
		currentIndex += singleMote[0].length();
		
		port = DEFAULT_SF_PORT;
		try {
		    port = Integer.parseInt(singleMote[1].replaceAll(" ",""));
		}
		catch (Exception e) {
		    throw new ParseException("ERROR: bad port \"" +
					     singleMote[1] + "\"",
					     currentIndex);
		}
		currentIndex += singleMote[1].length();
		
		moteID = 0;
		try {
		    moteID = 
			Integer.parseInt(singleMote[2].replaceAll(" ",""));
		}
		catch (Exception e) {
		    throw new ParseException("ERROR: bad id \"" +
					     singleMote[2] + "\"",
					     currentIndex);
		}
		currentIndex += singleMote[2].length();

	    }
	    ++currentIndex;

	    MoteIF iface = null;
	    PhoenixSource phs = null;
	    try {
		phs = BuildSource.makePhoenix("sf@" + host + ":" + port,
					      null);
		phs.setPacketErrorHandler(this);
		iface = new MoteIF(phs);
	    }
	    catch (Exception e) {
		throw e;
	    }

	    if (iface != null) {
		System.out.println("added phs for mote id " + moteID);
		moteTable.put(new Integer(moteID),iface);
		CommanderMessageListener l = 
		    new CommanderMessageListener(cc,iface,moteID);
		iface.registerListener(new PktCmd(),l);
		iface.registerListener(new PktRadioRecv(),l);
		moteMessageListeners.put(new Integer(moteID),l);
	    }

	}
    }

    // impl the phoenix error iface
    public void error(java.io.IOException e) {
	e.printStackTrace();
    }

    // impl the Commander iface
    public int sendCmd(PktCmd m, int moteID) throws IOException {
	MoteIF iface = (MoteIF)moteTable.get(new Integer(moteID));
	if (iface != null) {
	    iface.send(0x1,(Message)m);
	    return Commander.SUCCESS;
	}
	else {
	    return Commander.ERROR;
	}
    }

    public int sendMessage(Message m, int moteID) {
	return Commander.ERROR;
    }

    public Vector getMoteList() {
	Vector retval = new Vector();
	for (Enumeration e1 = moteTable.keys(); e1.hasMoreElements(); ) {
	    retval.add(e1.nextElement());
	}
	return retval;
    }

    public int removeMote(int moteID) {
	return Commander.ERROR;
    }
	
    public int addMote(String spec) {
	return Commander.ERROR;
    }

    public int addMote(String host,int port) {
	return Commander.ERROR;
    }

    public int disconnect() {
	return Commander.ERROR;
    }
    public int disconnect(int moteID) {
	return Commander.ERROR;
    }
    public int connect() {
	return Commander.ERROR;
    }
    public int connect(int moteID) {
	return Commander.ERROR;
    }

}



    
		
	
