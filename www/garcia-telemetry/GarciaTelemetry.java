/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.Date;

import java.io.IOException;

import java.net.URL;
import java.net.URLEncoder;

import java.applet.Applet;

import java.text.DecimalFormat;
import java.text.SimpleDateFormat;

import thinlet.Thinlet;
import thinlet.NumberFormatter;

import net.emulab.mtp_garcia_telemetry;

public class GarciaTelemetry
    extends Thinlet
{
    public static final SimpleDateFormat TIME_FORMAT =
	new SimpleDateFormat("hh:mm:ss a - ");
    
    public static final NumberFormatter FLOAT_FORMAT =
	new NumberFormatter(new DecimalFormat("0.00"));

    private final Applet applet;
    private URL servicePipe;
    
    public Object status;
    public Object lastUpdate;
    
    public Object log;
    public String logString = "";

    public mtp_garcia_telemetry mgt = new mtp_garcia_telemetry();

    public UpdateThread ut;
    
    public GarciaTelemetry(Applet applet)
	throws Exception
    {
	this.applet = applet;
	
	try
	{
	    this.add(this.parse("main.xml"));

	    URL urlServer = applet.getCodeBase();
	    String uid, auth, pipeurl;

	    /* Get our parameters then */
	    uid = applet.getParameter("uid");
	    auth = applet.getParameter("auth");
	    pipeurl = applet.getParameter("pipeurl");

	    /* ... form the URL that we will connect to. */
	    this.servicePipe = new URL(urlServer,
				       pipeurl
				       + "&nocookieuid="
				       + URLEncoder.encode(uid)
				       + "&nocookieauth="
				       + URLEncoder.encode(auth)
				       + "&service=telemetry");

	    this.connect();
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
    }

    private void connect()
	throws IOException
    {
	this.tkv.setKeyValue(this,
			     "ut",
			     new UpdateThread(this, this.servicePipe));
	this.ut.start();
    }

    public void reconnect()
	throws IOException
    {
	this.setString(this.status, "text", "Connecting...");
	this.connect();
    }

    public void appendLog(String msg)
    {
	this.logString += TIME_FORMAT.format(new Date()) + msg;
	this.setString(this.log, "text", this.logString);
	this.setInteger(this.log, "start", this.logString.length());
	this.setInteger(this.log, "end", this.logString.length());
    }

    public String toString()
    {
	return "GarciaTelemetry["
	    + "]";
    }
}
