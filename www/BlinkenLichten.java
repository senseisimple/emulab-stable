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
    private boolean on;
    
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
	byte buffer[] = new byte[1];
	
	try
	{
	    /* Just read a character at a time from the other side. */
	    while( this.is.read(buffer) > 0 )
	    {
		/* 1 == on, 0 == off */
		this.on = (buffer[0] == '1');
		
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

	/*
	 * Just paint the entire canvas provided to the applet, no need to get
	 * fancy.
	 */
	if( this.on )
	    g.setColor(Color.red);
	else
	    g.setColor(Color.black);
	g.fillRect(0, 0, size.width, size.height);
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
