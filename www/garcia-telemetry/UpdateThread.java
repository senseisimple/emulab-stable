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

import thinlet.ThinletKeyValue;

import net.emulab.mtp;
import net.emulab.mtp_packet;
import net.emulab.mtp_role_t;
import net.emulab.mtp_control;
import net.emulab.mtp_payload;
import net.emulab.mtp_opcode_t;
import net.emulab.mtp_status_t;
import net.emulab.mtp_command_goto;
import net.emulab.mtp_robot_type_t;
import net.emulab.mtp_contact_report;
import net.emulab.mtp_update_position;
import net.emulab.mtp_garcia_telemetry;

public class UpdateThread
    extends Thread
{
    private static final String STATUS_STRINGS[] = {
	"unknown",
	"",
	"idle",
	"moving",
	"error",
	"complete",
	"contact",
	"aborted"
    };
    
    private static final DecimalFormat LOG_FLOAT_FORMAT =
	new DecimalFormat("0.00");
    
    private final GarciaTelemetry gt;
    private final ThinletKeyValue tkv;
    private final URL servicePipe;
    private final mtp_packet mp = new mtp_packet();
    
    public UpdateThread(GarciaTelemetry gt, URL servicePipe)
	throws IOException
    {
	this.gt = gt;
	this.tkv = gt.getNamespace();
	this.servicePipe = servicePipe;
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
		    this.gt.setString(this.gt.status, "text", "Connected");
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
		    this.tkv.setKeyValue(this.gt,
					 "mgt",
					 this.mp.data.telemetry.garcia);
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_COMMAND_GOTO) {
		    mtp_command_goto mcg = this.mp.data.command_goto;
		    
		    this.gt.appendLog(
			"GOTO x="
			+ LOG_FLOAT_FORMAT.format(mcg.position.x)
			+ " y="
			+ LOG_FLOAT_FORMAT.format(mcg.position.y)
			+ " theta="
			+ LOG_FLOAT_FORMAT.format(mcg.position.theta)
			+ " id="
			+ this.mp.data.command_goto.command_id
			+ "\n");
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_COMMAND_STOP) {
		    this.gt.appendLog("STOP\n");
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_UPDATE_POSITION) {
		    mtp_update_position mup = this.mp.data.update_position;
		    int ms = mup.status + 1;

		    this.gt.appendLog(
			"UPDATE-POS "
			+ STATUS_STRINGS[ms]
			+ " x="
			+ LOG_FLOAT_FORMAT.format(mup.position.x)
			+ " y="
			+ LOG_FLOAT_FORMAT.format(mup.position.y)
			+ " theta="
			+ LOG_FLOAT_FORMAT.format(mup.position.theta)
			+ " id="
			+ this.mp.data.update_position.command_id
			+ "\n");
		}
		else if (this.mp.data.opcode ==
			 mtp_opcode_t.MTP_CONTACT_REPORT) {
		    mtp_contact_report mcr = this.mp.data.contact_report;
		    String msg;
		    int lpc;
		    
		    msg = "CONTACT-REPORT count=" + mcr.count + "\n";
		    for (lpc = 0; lpc < mcr.count; lpc++) {
			msg += "    x="
			    + LOG_FLOAT_FORMAT.format(mcr.points[lpc].x)
			    + " y="
			    + LOG_FLOAT_FORMAT.format(mcr.points[lpc].y)
			    + "\n";
		    }
		    this.gt.appendLog(msg);
		}
		else {
		    System.out.println("Unknown packet type: "
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

	this.tkv.setKeyValue(this.gt, "ut", null);
	this.gt.setString(this.gt.status, "text", "Disconnected");
	this.gt.appendLog("Disconnected\n");
    }

    public String toString()
    {
	return "UpdateThread["
	    + "]";
    }
}
