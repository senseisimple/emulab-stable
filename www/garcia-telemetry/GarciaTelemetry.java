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

import java.text.SimpleDateFormat;

import thinlet.Thinlet;

public class GarciaTelemetry
    extends Thinlet
{
    public static final SimpleDateFormat TIME_FORMAT =
	new SimpleDateFormat("hh:mm:ss a - ");
    
    private final Applet applet;
    private URL servicePipe;
    
    public Object batteryBar;
    public Object batteryText;
    
    public Object leftOdometer;
    public Object rightOdometer;
    
    public Object leftVelocity;
    public Object rightVelocity;
    
    public Object flSensor;
    public Object frSensor;
    public Object slSensor;
    public Object srSensor;
    public Object rlSensor;
    public Object rrSensor;

    public Object log;
    public String logString = "";
    
    public Object connected;
    public Object lastUpdate;
    public Object reconnectButton;

    public GarciaTelemetry(Applet applet)
	throws Exception
    {
	this.applet = applet;
	
	try
	{
	    this.add(this.parse("main.xml"));

	    this.batteryBar = this.find("battery-bar");
	    this.batteryText = this.find("battery-text");
	    
	    this.leftOdometer = this.find("left-odom");
	    this.rightOdometer = this.find("right-odom");
	    
	    this.leftVelocity = this.find("left-vel");
	    this.rightVelocity = this.find("right-vel");
	    
	    this.flSensor = this.find("fl-sensor");
	    this.frSensor = this.find("fr-sensor");

	    this.slSensor = this.find("sl-sensor");
	    this.srSensor = this.find("sr-sensor");

	    this.rlSensor = this.find("rl-sensor");
	    this.rrSensor = this.find("rr-sensor");

	    this.log = this.find("log");

	    this.connected = this.find("connected");
	    this.lastUpdate = this.find("last-update");
	    this.reconnectButton = this.find("reconnect-button");
	    
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
	new UpdateThread(this, this.servicePipe).start();
    }

    public void reconnect()
	throws IOException
    {
	this.setBoolean(this.reconnectButton, "enabled", false);
	this.setString(this.connected, "text", "Connecting...");
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
