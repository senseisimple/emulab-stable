/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.io.IOException;
import java.io.PrintStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;

import java.net.URL;
import java.net.URLConnection;

import java.text.DecimalFormat;

import org.acplt.oncrpc.XdrTcpDecodingStream;
import org.acplt.oncrpc.XdrTcpEncodingStream;

import net.emulab.mtp;
import net.emulab.mtp_packet;
import net.emulab.mtp_role_t;
import net.emulab.mtp_control;
import net.emulab.mtp_payload;
import net.emulab.mtp_opcode_t;
import net.emulab.mtp_status_t;
import net.emulab.mtp_robot_type_t;
import net.emulab.mtp_garcia_telemetry;

public class UpdateThread
    extends Thread
{
    private static final DecimalFormat FLOAT_FORMAT =
	new DecimalFormat("0.00");

    private static final String STATUS_STRINGS[] = {
	"unknown",
	"",
	"idle",
	"moving",
	"error",
	"complete",
	"obstructed",
	"aborted"
    };
    
    private final GarciaTelemetry gt;
    private final URL servicePipe;
    private final mtp_packet mp = new mtp_packet();
    
    public UpdateThread(GarciaTelemetry gt, URL servicePipe)
	throws IOException
    {
	this.gt = gt;
	this.servicePipe = servicePipe;
    }

    private void updateGUI(mtp_garcia_telemetry mgt)
    {
	this.gt.setInteger(this.gt.batteryBar,
			   "value",
			   (int)mgt.battery_level);
	this.gt.setString(this.gt.batteryText,
			  "text",
			  Integer.toString((int)mgt.battery_level) + "%");
		    
	this.gt.setString(this.gt.leftOdometer,
			  "text",
			  FLOAT_FORMAT.format(mgt.left_odometer) + " meters");
	this.gt.setString(this.gt.rightOdometer,
			  "text",
			  FLOAT_FORMAT.format(mgt.right_odometer) + " meters");
		    
	this.gt.setString(this.gt.leftVelocity,
			  "text",
			  FLOAT_FORMAT.format(mgt.left_velocity) + " m/s");
	this.gt.setString(this.gt.rightVelocity,
			  "text",
			  FLOAT_FORMAT.format(mgt.right_velocity) + " m/s");
		    
	this.gt.setString(this.gt.flSensor,
			  "text",
			  FLOAT_FORMAT.format(mgt.front_ranger_left)
			  + " meters");
	this.gt.setString(this.gt.frSensor,
			  "text",
			  FLOAT_FORMAT.format(mgt.front_ranger_right)
			  + " meters");
	this.gt.setString(this.gt.slSensor,
			  "text",
			  FLOAT_FORMAT.format(mgt.side_ranger_left)
			  + " meters");
	this.gt.setString(this.gt.srSensor,
			  "text",
			  FLOAT_FORMAT.format(mgt.side_ranger_right)
			  + " meters");
	this.gt.setString(this.gt.rlSensor,
			  "text",
			  FLOAT_FORMAT.format(mgt.rear_ranger_left)
			  + " meters");
	this.gt.setString(this.gt.rrSensor,
			  "text",
			  FLOAT_FORMAT.format(mgt.rear_ranger_right)
			  + " meters");
    }
    
    public void run()
    {
	PrintStream ps = null;
	InputStream is = null;
	
	try {
	    boolean connected = false, looping = true;
	    long lastPacketTime, currentTime;
	    XdrTcpEncodingStream bxdr;
	    XdrTcpDecodingStream xdr;
	    ByteArrayOutputStream baos;
	    URLConnection uc;
	    mtp_packet init;
	    String request;
	    
	    this.gt.appendLog("Connecting...\n");
	    
	    uc = this.servicePipe.openConnection();
	    uc.setDoInput(true);
	    uc.setDoOutput(true);
	    uc.setUseCaches(false);

	    init = new mtp_packet();
	    init.vers = (short)mtp.MTP_VERSION;
	    init.role = (short)mtp_role_t.MTP_ROLE_EMULAB;
	    init.data = new mtp_payload();
	    init.data.opcode = mtp_opcode_t.MTP_CONTROL_INIT;
	    init.data.init = new mtp_control();
	    init.data.init.msg = "garcia-telemetry v0.1";
	    
	    baos = new ByteArrayOutputStream(128);
	    bxdr = new XdrTcpEncodingStream(null, baos, 128);
	    bxdr.beginEncoding(null, 0);
	    init.xdrEncode(bxdr);
	    bxdr.endEncoding(true);

	    ps = new PrintStream(uc.getOutputStream());
	    ps.print("init=");
	    ps.print(Base64.encodeBytes(baos.toByteArray()));
	    ps.println();
	    ps.close();
	    ps = null;
	    
	    is = uc.getInputStream();
	    xdr = new XdrTcpDecodingStream(null, is, 2048);
	    
	    lastPacketTime = currentTime = System.currentTimeMillis();
	    while (looping) {
		xdr.beginDecoding();
		this.mp.xdrDecode(xdr);
		xdr.endDecoding();
		if (!connected) {
		    this.gt.appendLog("Connected\n");
		    this.gt.setString(this.gt.connected, "text", "Connected");
		    connected = true;
		}
		
		currentTime = System.currentTimeMillis();
		this.gt.setString(this.gt.lastUpdate,
				  "text",
				  Long.toString(currentTime - lastPacketTime));
		lastPacketTime = currentTime;
		
		if ((this.mp.data.opcode == mtp_opcode_t.MTP_TELEMETRY) &&
		    (this.mp.data.telemetry.type ==
		     mtp_robot_type_t.MTP_ROBOT_GARCIA)) {
		    this.updateGUI(this.mp.data.telemetry.garcia);
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_COMMAND_GOTO) {
		    this.gt.appendLog(
			"GOTO x="
			+ this.mp.data.command_goto.position.x
			+ " y="
			+ this.mp.data.command_goto.position.y
			+ " theta="
			+ this.mp.data.command_goto.position.theta
			+ "\n");
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_COMMAND_STOP) {
		    this.gt.appendLog("STOP\n");
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_UPDATE_POSITION) {
		    int ms = this.mp.data.update_position.status + 1;

		    this.gt.appendLog("UPDATE-POS "
				      + STATUS_STRINGS[ms]
				      + "\n");
		}
		else {
		    System.err.println("Unknown packet type: "
				       + this.mp.data.opcode);
		}
	    }
	}
	catch(Throwable th) {
	    th.printStackTrace();
	}
	try {
	    if (ps != null)
		ps.close();
	    if (is != null)
		is.close();
	}
	catch(Throwable th) {
	    th.printStackTrace();
	}
	
	this.gt.setBoolean(this.gt.reconnectButton, "enabled", true);
	this.gt.setString(this.gt.connected, "text", "Disconnected");
	this.gt.appendLog("Disconnected\n");
    }

    public String toString()
    {
	return "UpdateThread["
	    + "]";
    }
}
