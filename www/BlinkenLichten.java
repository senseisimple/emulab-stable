/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Dimension;

import java.applet.Applet;

import java.io.IOException;
import java.io.InputStream;

import java.net.URL;
import java.net.URLEncoder;
import java.net.URLConnection;

/**
 * An applet that can be used to reflect the status of an LED on a mote.  It
 * will connect to a URL on boss and read in characters that should represent
 * the current status of the LED.
 */
public class BlinkenLichten
	extends Applet
	implements Runnable
{
    /**
     * The connection to boss that will provide LED status values.
     */
    private InputStream is;

    /**
     * The current status of the light, just on/off for now.
     */
    private boolean red_on;
    private boolean green_on;
    private boolean yellow_on;
    
    public BlinkenLichten()
    {
    }

    public void init()
    {
	setBackground(Color.black);
	setForeground(Color.red);
	
	try
	{
	    URL urlServer = this.getCodeBase(), ledpipe;
	    String uid, auth, pipeurl;
	    URLConnection uc;

	    /* Get our parameters then */
	    uid = this.getParameter("uid");
	    auth = this.getParameter("auth");
	    pipeurl = this.getParameter("pipeurl");

	    /* ... form the URL that we will connect to. */
	    ledpipe = new URL(urlServer,
			      pipeurl
			      + "&nocookieuid="
			      + URLEncoder.encode(uid)
			      + "&nocookieauth="
			      + URLEncoder.encode(auth));

	    uc = ledpipe.openConnection();
	    uc.setDoInput(true);
	    uc.setUseCaches(false);
	    this.is = uc.getInputStream();
	    
	    new Thread(this).start();
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
    }
    
    public void run()
    {
	byte buffer[] = new byte[6];
	
	try
	{
	    /* Just read a character at a time from the other side. */
	    // Bad, bad, bad
	    //while( this.is.read(buffer) > 0 )
	    while( this.is.read(buffer,0,6) > 0 )
	    {
		/* 1 == on, 0 == off */
		this.red_on = (buffer[0] == '1');
		this.green_on = (buffer[2] == '1');
		this.yellow_on = (buffer[4] == '1');
		
		repaint();
	    }
	}
	catch(IOException e)
	{
	    e.printStackTrace();
	}
	
	this.is = null;
    }
    
    public void update(Graphics g)
    {
	paint(g);
    }
    
    public void paint(Graphics g)
    {
	Dimension size = getSize();
	int width = size.width / 3;
	int height = size.height;

	/*
	 * Paint each of the three LED values
	 */
	if (this.red_on)
	    g.setColor(Color.red);
	else
	    g.setColor(Color.red.darker().darker());
	g.fillRect(0, 0, width, height);

	if (this.green_on)
	    g.setColor(Color.green);
	else
	    g.setColor(Color.green.darker().darker());
	g.fillRect(width, 0, width, height);

	if (this.yellow_on)
	    g.setColor(Color.yellow);
	else
	    g.setColor(Color.yellow.darker().darker());
	g.fillRect(width *2, 0, width, height);
    }
    
    public void destroy()
    {
	try
	{
	    if( this.is != null)
		this.is.close();
	}
	catch(IOException e)
	{
	    e.printStackTrace();
	}
    }
}
