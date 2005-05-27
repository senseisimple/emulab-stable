package net.emulab.mm;

import net.emulab.packet.PktCmd;
import net.emulab.packet.PktRadioRecv;

import java.util.Vector;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.PrintStream;
import java.io.IOException;

public class NNClient extends Thread implements CommanderClient {

    // the command number; useful to find out who's returned and who hasn't.
    public static short commandSeqNo = 1;

    // our next hop to the serial forwarder stream
    private MoteCommander mc;
    // write debug msgs and data out.
    // all debug messages start with "DEBUG: "
    // other messages are computed data output.
    private PrintStream logger;

    // contains commands (key == cmd_no, value == PktCmd)
    // updates when commands are sent, and replaces value with mote
    // return value when command completes -- this gives command status
    // to user.
    public Hashtable commands;
    // contains packets in between power level mods 
    public Hashtable recvPkt;
    // contains a map of power level values to received packets
    // basically, after every power level packet test, the recvPkt hashtable
    // is copied into this hash.
    public Hashtable powerLevelRecvPkt;
    // default; can be overridden on the cmd line
    private int pktCount = 2;

    public NNClient(String motes,PrintStream logger) throws Exception {
	try {
	    this.mc = new MoteCommander(this,motes);
	}
	catch (Exception e) {
	    throw e;
	}
	this.logger = logger;
	this.commands = new Hashtable();
	this.recvPkt = new Hashtable();
	this.powerLevelRecvPkt = new Hashtable();
    }

    public NNClient(String hostname,int port,
		    String motes,PrintStream logger) throws Exception {
	try {
	    this.mc = new MoteCommander(this,hostname,port,motes);
	}
	catch (Exception e) {
	    throw e;
	}
	this.logger = logger;
	this.commands = new Hashtable();
	this.recvPkt = new Hashtable();
	this.powerLevelRecvPkt = new Hashtable();
    }

    public NNClient(String hostname,int port,
		    String motes,int pktCount,
		    PrintStream logger) throws Exception {
	try {
	    this.mc = new MoteCommander(this,hostname,port,motes);
	}
	catch (Exception e) {
	    throw e;
	}
	this.logger = logger;
	this.commands = new Hashtable();
	this.recvPkt = new Hashtable();
	this.pktCount = pktCount;
	this.powerLevelRecvPkt = new Hashtable();
    }

    // impl the CommanderClient iface
    public synchronized void cmdResponse(PktCmd m,int moteID) {

	String status;

// 	if (m.get_cmd_status() == Commander.CMD_STATUS_SUCCESS) {
// 	    status = "success";
// 	}
// 	else if (m.get_cmd_status() == Commander.CMD_STATUS_ERROR) {
// 	    status = "error";
// 	}
// 	else if (m.get_cmd_status() == Commander.CMD_STATUS_UNKNOWN) {
// 	    status = "unknown";
// 	}
// 	else {
// 	    status = "badstatus";
// 	}

//  	logger.println("PktCmd from mote " + moteID + 
//  		       " (status: " + status + ")");

// 	if (m.get_cmd_type() == Commander.CMD_TYPE_SET_RF_POWER && 
// 	    m.get_cmd_status() == Commander.CMD_STATUS_SUCCESS) {
// 	    logger.println("PktCmd from mote " + moteID + 
// 			   " (status: " + status + "):\n" + m.toString());
// 	}

	commands.put(new Integer(m.get_cmd_no()),m);

	this.notifyAll();
    }

    public synchronized int getCmdStatus(int cmd) {
	PktCmd c = (PktCmd)commands.get(new Integer(cmd));
	if (c == null) {
	    return Commander.CMD_STATUS_UNKNOWN;
	}
	else {
	    return c.get_cmd_status();
	}
    }

    public synchronized void radioRecv(PktRadioRecv rr,int moteID) {
//  	logger.println("PktRadioRecv from mote " + moteID + 
//  		       ":\n" + rr.toString());

	Integer mid = new Integer(moteID);
	Hashtable h = (Hashtable)recvPkt.get(mid);
	if (h == null) {
	    h = new Hashtable();
	    recvPkt.put(mid,h);
	}
	Integer sid = new Integer(rr.get_bcast_src_mote_id());
	Vector v = (Vector)h.get(sid);
	if (v == null) {
	    v = new Vector();
	    h.put(sid,v);
	}
	v.add(rr);

	//this.notifyAll();
    }

//     public Commander getCommander() {
// 	return (Commander)mc;
//     }

    public int getPktCount() {
	return pktCount;
    }

    public synchronized int sendCmd(PktCmd c,int moteID) throws IOException {
	commands.put(new Integer(c.get_cmd_no()),c);
	return this.mc.sendCmd(c,moteID);
    }

    public void debug(String s) {
	logger.println("DEBUG: "+s);
    }

    public synchronized void run() {
	// do the sequencing...
	Vector motes = mc.getMoteList();

// 	for (Enumeration e1 = motes.elements(); e1.hasMoreElements(); ) {
// 	    int moteID = (int)(((Integer)e1.nextElement()).intValue());
// 	    PktCmd c = new PktCmd();
// 	    c.set_cmd_type(Commander.CMD_TYPE_SET_MOTE_ID);
// 	    c.set_cmd_no(commandSeqNo++);
// 	    c.set_cmd_status(Commander.CMD_STATUS_UNKNOWN);
// 	    c.set_mote_id((short)moteID);

// 	    try {
// 		mc.sendCmd(c,moteID);
// 	    }
// 	    catch (Exception e) {
// 		e.printStackTrace();
// 	    }

// 	    try {
// 		this.wait();
// 	    }
// 	    catch (Exception e) {
// 		e.printStackTrace();
// 	    }

// 	}

// 	for (Enumeration e1 = motes.elements(); e1.hasMoreElements(); ) {
// 	    int moteID = (int)(((Integer)e1.nextElement()).intValue());
// 	    PktCmd c = new PktCmd();
// 	    c.set_cmd_type(Commander.CMD_TYPE_RADIO_OFF);
// 	    c.set_cmd_no(commandSeqNo++);
// 	    c.set_cmd_status(Commander.CMD_STATUS_UNKNOWN);

// 	    try {
// 		mc.sendCmd(c,moteID);
// 	    }
// 	    catch (Exception e) {
// 		e.printStackTrace();
// 	    }

// 	    try {
// 		this.wait();
// 	    }
// 	    catch (Exception e) {
// 		e.printStackTrace();
// 	    }
// 	}

// 	for (Enumeration e1 = motes.elements(); e1.hasMoreElements(); ) {
// 	    int moteID = (int)(((Integer)e1.nextElement()).intValue());
// 	    PktCmd c = new PktCmd();
// 	    c.set_cmd_type(Commander.CMD_TYPE_RADIO_ON);
// 	    c.set_cmd_no(commandSeqNo++);
// 	    c.set_cmd_status(Commander.CMD_STATUS_UNKNOWN);

// 	    try {
// 		mc.sendCmd(c,moteID);
// 	    }
// 	    catch (Exception e) {
// 		e.printStackTrace();
// 	    }

// 	    try {
// 		this.wait();
// 	    }
// 	    catch (Exception e) {
// 		e.printStackTrace();
// 	    }
// 	}

	short[] data = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9 };
// 	short[] powerLevels = { 0x2,
// 				0x3,0x4,0x5,0x6,0x7,0x8,0x9,
// 				0xb,0xc,0xd,0xf,
// 				0x40,0x50,0x60,0x70,0x80,0x90,0xb0,0xc0,
// 				0xf0,
// 				0xff };

        // from contrib/xbow/tools/src/xlisten/boards/mica2.c
	// int table_916MHz_power[21][2] = {{2,-20},{4,-16},{5,-14},{6,-13},
	//       {7,-12},{8,-11},{9,-10},{11,-9},{12,-8},
	//       {13,-7},{15,-6},{64,-5},{80,-4},{96,-2},{112,-1},{128,0},
	//       {144,1},{176,2},{192,3},{240,4},{255,5}}      ;

	short[] powerLevels = { 2,4,5,6,7,8,9,11,12,13,15,
				64,80,96,112,128,144,176,192,240,255 };
	int status;
	String statusStr;

	for (int i = 0; i < powerLevels.length; ++i) {
	    debug("setting power " + powerLevels[i]);

	    for (Enumeration e1 = motes.elements(); e1.hasMoreElements(); ) {
		int moteID = (int)(((Integer)e1.nextElement()).intValue());
		PktCmd c = new PktCmd();
		c.set_cmd_type(Commander.CMD_TYPE_SET_RF_POWER);
		c.set_cmd_no(++commandSeqNo);
		c.set_cmd_status(Commander.CMD_STATUS_UNKNOWN);
		c.set_power((short)powerLevels[i]);

		try {
		    this.sendCmd(c,moteID);
		}
		catch (Exception e) {
		    e.printStackTrace();
		}
		
		try {
		    this.wait();
		}
		catch (Exception e) {
		    e.printStackTrace();
		}

		status = getCmdStatus(commandSeqNo);
		if (status == Commander.CMD_STATUS_SUCCESS) {
		    statusStr = "success";
		}
		else if (status == Commander.CMD_STATUS_ERROR) {
		    statusStr = "error";
		}
		else if (status == Commander.CMD_STATUS_UNKNOWN) {
		    statusStr = "unknown";
		}
		else {
		    statusStr = "badstatus";
		}
		
		debug("set power cmd(" + commandSeqNo + ") to mote" + moteID + 
		      " " + statusStr);
		
	    }
	    
	    debug("sending bcast cmds");

	    for (Enumeration e1 = motes.elements(); e1.hasMoreElements(); ) {
		int moteID = (int)(((Integer)e1.nextElement()).intValue());
		PktCmd c = new PktCmd();
		
		c.set_cmd_type(Commander.CMD_TYPE_BCAST_MULT);
		c.set_cmd_no(++commandSeqNo);
		c.set_cmd_status(Commander.CMD_STATUS_UNKNOWN);
		c.set_cmd_flags(Commander.CMD_FLAGS_INCR_FIRST);
		c.set_bcast_src_mote_id(moteID);
		c.set_bcast_dest_mote_id(0);
		c.set_bcast_data(data);
		c.set_msg_dup_count(getPktCount());
		c.set_interval(128);
		
		try {
		    // go through us to log that we sent a command
		    this.sendCmd(c,moteID);
		}
		catch (Exception e) {
		    e.printStackTrace();
		}
		
		try {
		    this.wait();
		}
		catch (Exception e) {
		    e.printStackTrace();
		}

		status = getCmdStatus(commandSeqNo);
		if (status == Commander.CMD_STATUS_SUCCESS) {
		    statusStr = "success";
		}
		else if (status == Commander.CMD_STATUS_ERROR) {
		    statusStr = "error";
		}
		else if (status == Commander.CMD_STATUS_UNKNOWN) {
		    statusStr = "unknown";
		}
		else {
		    statusStr = "badstatus";
		}

		if (status == Commander.CMD_STATUS_SUCCESS) {
		    // sleep for 10 s, to give in-the-air packets time to get 
		    // to their destinations, then move along.

		    try {
			this.wait(1000);
		    }
		    catch (Exception e) {
			e.printStackTrace();
		    }

		}
		
		debug("sent bcast cmd(" + commandSeqNo + ") to mote" + 
		      moteID + " " + statusStr);

	    }

	    debug("sleeping for 2s before changing to next power level");
	    try {
		this.wait(2000);
	    }
	    catch (Exception e) {
		e.printStackTrace();
	    }


	    // do the data manip now, instead of later...
	    logger.println("\nData for power level 0x" + 
			   Integer.toHexString(powerLevels[i]) + 
			   "");
	    
	    for (Enumeration e2 = recvPkt.keys(); 
		 e2.hasMoreElements(); 
		 ) {
		Integer receivingMote = (Integer)e2.nextElement();
		Hashtable rmPkts = (Hashtable)recvPkt.get(receivingMote);
// 		logger.println("  <RP desc=\"Received Pkt stats for mote " + 
// 			       receivingMote.intValue() +
// 			       "\">");
	    
		if (rmPkts == null) {
		    //logger.println("0 packets received.");
		}
		else {
		    // first count how many total packets were received:
		    // we do this for all mote ids, not just those we recv from
		    // (might make parsing easier...)
		    // that is, we do it for all motes NOT INCLUDING ourself!!!
		    Vector moteIDs = mc.getMoteList();
		    for (Enumeration e3 = moteIDs.elements(); 
			 e3.hasMoreElements(); 
			 ) {
			Integer srcMote = (Integer)e3.nextElement();
			if (srcMote.intValue() != receivingMote.intValue()) {
			    Vector pktList = (Vector)rmPkts.get(srcMote);
			    
			    int numPkts = 0;
			    float avgRSSI = 0.0f;
			    float minRSSI = Float.MAX_VALUE;
			    float maxRSSI = Float.MIN_VALUE;
			    float stdDevRSSI = 0.0f;
			    float pktRecvPct = 0.0f;

			    // mica2 rssi conversion:
			    // Vrssi=Vbat * ADC_value/1024
			    // RSSI(dbm)=-51.3 * Vrssi -49.2 for 433Mhz and
			    // RSSI(dbm)=-50 *Vrssi -45.5 for 915Mhz

			    // Vbat when plugged into the serial board?
			    // according to the MPR/MIB manual from xbow, 
			    // the mib510 sends regulated 3 VDC to mote from
			    // wall.

			    if (pktList != null && pktList.size() != 0) {
				numPkts = pktList.size();
				
				Vector rssiVals = new Vector();
				
				for (Enumeration e4 = pktList.elements();
				     e4.hasMoreElements();
				     ) {
				    PktRadioRecv c = 
					(PktRadioRecv)e4.nextElement();
				    float vRSSI = 
					3.0f*((float)c.get_rssi()/1024);
				    float tmpRSSI = -50.0f*vRSSI - 45.5f;
				    avgRSSI += tmpRSSI;
				    rssiVals.add(new Float(tmpRSSI));

				    if (tmpRSSI < minRSSI) {
					minRSSI = tmpRSSI;
				    }

				    if (tmpRSSI > maxRSSI) {
					maxRSSI = tmpRSSI;
				    }
					

				}
				
				avgRSSI /= numPkts;

				float sum = 0.0f;
				for (Enumeration e4 = rssiVals.elements();
				     e4.hasMoreElements();
				     ) {
				    sum += (((Float)e4.nextElement()).floatValue()) - avgRSSI;
				}
				stdDevRSSI = sum/numPkts;
				
				pktRecvPct = 
				    100.0f * ((float)numPkts)/getPktCount();
				
			    }

			    if (minRSSI == Float.MAX_VALUE) {
				minRSSI = 0.0f;
			    }
			    if (maxRSSI == Float.MIN_VALUE) {
				maxRSSI = 0.0f;
			    }
			    
			    logger.println(
					   "(" + receivingMote.intValue() + 
					   ") <- (" + 
					   srcMote.intValue() + "): pkts(" + 
					   numPkts + "/" + getPktCount() + 
					   "," + pktRecvPct + 
					   "%) rssi( " + minRSSI + "," + 
					   maxRSSI + "," + avgRSSI + "," + 
					   stdDevRSSI + ")");
			}

		    }
		    
		}

	    }

	    //logger.println("#End PL#");


	    // copy packets received on this power level to the right table.
	    powerLevelRecvPkt.put(new Integer(powerLevels[i]),
				  recvPkt);
	    recvPkt = new Hashtable();
	    debug("finished power level " + powerLevels[i]);


	}

// 	// print out the command hash:
// 	for (Enumeration e1 = commands.keys(); e1.hasMoreElements(); ) {
// 	    Integer key = (Integer)e1.nextElement();
// 	    PktCmd c = (PktCmd)commands.get(key);
// // 	    String statusStr;

// 	    status = c.get_cmd_status();
// 	    if (status == Commander.CMD_STATUS_SUCCESS) {
// 		statusStr = "success";
// 	    }
// 	    else if (status == Commander.CMD_STATUS_ERROR) {
// 		statusStr = "error";
// 	    }
// 	    else if (status == Commander.CMD_STATUS_UNKNOWN) {
// 		statusStr = "unknown";
// 	    }
// 	    else {
// 		statusStr = "badstatus";
// 	    }

// 	    debug("key = "+key.intValue()+"; value.status = "+statusStr);
// 	}
	

	// look at all the packets... and print packet counts and rssi
// 	for (Enumeration e1 = powerLevelRecvPkt.keys(); 
// 	     e1.hasMoreElements(); 
// 	     ) {
// 	    Integer powerLevel = (Integer)e1.nextElement();
// 	    Hashtable recvPackets = 
// 		(Hashtable)powerLevelRecvPkt.get(powerLevel);

// 	    logger.println("#Start PL# Data for power level " + 
// 			   Integer.toHexString(powerLevel.intValue()));

// 	    for (Enumeration e2 = recvPackets.keys(); 
// 		 e2.hasMoreElements(); 
// 		 ) {
// 		Integer receivingMote = (Integer)e2.nextElement();
// 		Hashtable rmPkts = (Hashtable)recvPackets.get(receivingMote);
// 		logger.println("#Start RP# Received Pkt Stats for mote " + 
// 			       receivingMote.intValue());
	    
// 		if (rmPkts == null) {
// 		    logger.println("0 packets received.");
// 		}
// 		else {
// 		    // first count how many total packets were received:
// 		    // we do this for all mote ids, not just those we recv from
// 		    // (might make parsing easier...)
// 		    // that is, we do it for all motes NOT INCLUDING ourself!!!
// 		    Vector moteIDs = mc.getMoteList();
// 		    for (Enumeration e3 = moteIDs.elements(); 
// 			 e3.hasMoreElements(); 
// 			 ) {
// 			Integer srcMote = (Integer)e3.nextElement();
// 			if (srcMote.intValue() != receivingMote.intValue()) {
// 			    Vector pktList = (Vector)rmPkts.get(srcMote);
			    
// 			    int numPkts = 0;
// 			    float avgRSSI = 0.0f;
			    

// 			    // mica2 rssi conversion:
// 			    // Vrssi=Vbat * ADC_value/1024
// 			    // RSSI(dbm)=-51.3 * Vrssi -49.2 for 433Mhz and
// 			    // RSSI(dbm)=-50 *Vrssi -45.5 for 915Mhz

// 			    // Vbat when plugged into the serial board?
// 			    // according to the MPR/MIB manual from xbow, 
// 			    // the mib510 sends regulated 3 VDC to mote from
// 			    // wall.

// 			    if (pktList != null) {
// 				numPkts = pktList.size();
				
				
// 				for (Enumeration e4 = pktList.elements();
// 				     e4.hasMoreElements();
// 				     ) {
// 				    PktRadioRecv c = 
// 					(PktRadioRecv)e4.nextElement();
// 				    float vRSSI = 
// 					3.0f*((float)c.get_rssi()/1024);
// 				    float tmpRSSI = -50.0f*vRSSI - 45.5f;
// 				    avgRSSI += tmpRSSI;
// 				}
				
// 				if (numPkts != 0) {
// 				    avgRSSI /= numPkts;
// 				}
				
// 				float pktRecvPct = 
// 				    ((float)numPkts)/getPktCount();
				
// 				logger.println("  - from mote " + 
// 					   srcMote.intValue() + ": " + 
// 					   numPkts + "/" + getPktCount() + 
// 					   " (" + pktRecvPct + " %) recv; " + 
// 					   avgRSSI + " average RSSI.");
// 			    }
// 			}

// 		    }
		    
// 		}
// 		logger.println("#End RP# Received Pkt Stats for mote " + 
// 			       receivingMote.intValue());
// 	    }

// 	    logger.println("#End PL# Data for power level " + 
// 			   Integer.toHexString(powerLevel.intValue()));

// 	}

	System.exit(0);
		
    }
	    


    public static void main(String[] args) {
	NNClient n;
	
	if (args.length != 1 && args.length != 3 && args.length != 4) {
	    System.err.println("Error: need one arg that specs a bunch of " +
			       "motes, or 3 or 4args.\n");
	    System.exit(-1);
	}

	if (args.length == 1) {
	
	    try {
		n = new NNClient(args[0],System.out);
		n.start();
	    }
	    catch (Exception e) {
		e.printStackTrace();
	    }
	}
	else if (args.length == 3) {
	    int port;

	    try {
		port = Integer.parseInt(args[1]);
		n = new NNClient(args[0],port,args[2],System.out);
		n.start();
	    }
	    catch (Exception e) {
		e.printStackTrace();
	    }
	}
	else if (args.length == 4) {
	    int port;
	    int pktCount;

	    try {
		port = Integer.parseInt(args[1]);
		pktCount = Integer.parseInt(args[3]);
		n = new NNClient(args[0],port,args[2],pktCount,System.out);
		n.start();
	    }
	    catch (Exception e) {
		e.printStackTrace();
	    }
	}
    }

}
