/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import javax.swing.*;
import javax.swing.event.*;
import java.net.URL;
import java.util.*;
import java.text.*;
import java.io.*;
import java.net.URL;
import java.net.URLEncoder;
import java.net.URLConnection;
import com.sun.image.codec.jpeg.*;

/*
 * A really stupid applet to display a motion jpeg.
 */
public class WebCamApplet extends JApplet {
    private WebCam		webcam;
    private BufferedImage	curimage = null;
    
    /*
     * The connection to boss that will provide the data stream.
     */
    private InputStream is;

    public void init() {
	try
	{
	    URL urlServer = this.getCodeBase(), webcamurl;
	    URLConnection uc;

	    // form the URL that we use to get image data.
	    webcamurl = new URL(urlServer, this.getParameter("URL"));

	    // And connect to it.
	    uc = webcamurl.openConnection();
	    uc.setDoInput(true);
	    uc.setUseCaches(false);
	    this.is = uc.getInputStream();
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}

	/*
	 * Add the single component to the pane.
	 */
        getContentPane().add(webcam = new WebCam());
    }

    public void start() {
        webcam.start();
    }
  
    public void stop() {
        webcam.stop();
    }

    private class WebCam extends JPanel implements Runnable {
        private Thread thread;
        private BufferedImage bimg;
	private Graphics2D G2 = null;

        public WebCam() {
        }

	/*
	 * I stole this from some demo programs. Not really sure what
	 * it does exactly, but has something to do with double buffering.
	 */
	public Graphics2D createGraphics2D(int w, int h) {
	    //System.out.println("createGraphics2D");

	    if (G2 == null) {
		bimg = (BufferedImage) createImage(w, h);
		
		G2 = bimg.createGraphics();
		G2.setBackground(getBackground());
		G2.setRenderingHint(RenderingHints.KEY_RENDERING,
				    RenderingHints.VALUE_RENDER_QUALITY);
	    }
	    return G2;
	}

	public void drawImage(int w, int h, Graphics2D g2) {
	    int fw, fh;

	    fw = curimage.getWidth(this);
	    fh = curimage.getHeight(this);

	    /*
	     * Shove the new image in.
	     */
	    g2.drawImage(curimage, 0, 0, fw, fh, this);
	}

	/*
	 * This is the callback to paint the map. 
	 */
        public void paint(Graphics g) {
            Dimension dim  = getSize();
	    int h = dim.height;
	    int w = dim.width;

	    if (curimage == null)
		return;
	    
	    if (false) {
		Graphics2D g2 = createGraphics2D(w, h);
		drawImage(w, h, g2);
		g.drawImage(bimg, 0, 0, this);
	    }
	    else {
		g.drawImage(curimage, 0, 0, this);
	    }
        }
    
        public void start() {
	    G2     = null;
            thread = new Thread(this);
            thread.start();
        }
    
        public synchronized void stop() {
            thread = null;
        }
    
        public void run() {
            Thread me = Thread.currentThread();
	    BufferedInputStream input = new BufferedInputStream(is);
	    String lenhdr = "content-length: ";
	    byte imageBytes[] = new byte[100000];
	    ByteArrayInputStream imageinput;
	    String str;
	    int size;

            while (thread == me) {
		try
		{
		    // First line is the boundry marker
		    if ((str = readLine(input).trim()) == null)
			break;

		    if (!str.equals("--myboundary")) {
			System.out.println("Sync error at boundry");
			break;
		    }

		    // Next line is the Content-type. Skip it.
		    if ((str = readLine(input)) == null)
			break;

		    // Next line is the Content-length. We need this. 
		    if ((str = readLine(input)) == null)
			break;
		    str = str.toLowerCase();

		    if (! str.startsWith(lenhdr)) {
			System.out.println("Sync error at content length");
			break;
		    }
		    str  = str.substring(lenhdr.length(), str.length());
		    size = Integer.parseInt(str.trim());
		    
		    if (size < 0) {
			System.out.println("Bad content length: " + size);
			break;
		    }
		    //System.out.println("content length: " + size);
		    
		    // Next line is a blank line. Skip it.
		    if ((str = readLine(input)) == null)
			break;

		    // Now we have the data. Buffer that up.
		    int count = 0;
		    while (count < size) {
			int cc = input.read(imageBytes, count, size - count);
			if (cc == -1)
			    break;

			count += cc;
		    }
		    if (count != size) {
			System.out.println("Not enough image data");
			break;
		    }
		    // For the jpeg decoder.
		    imageinput = new ByteArrayInputStream(imageBytes);
		    
		    JPEGImageDecoder decoder =
			JPEGCodec.createJPEGDecoder(imageinput);
		    
		    curimage = decoder.decodeAsBufferedImage();

		    //System.out.println("Got the image");
		    
		    // Next line is a blank line. Skip it.
		    if ((str = readLine(input)) == null)
			break;
		    
		    repaint();
		    me.yield();
		}
		catch(IOException e)
		{
		    e.printStackTrace();
		    break;
		}
            }
            thread = null;
        }

	/*
	 * Catch termination.
	 */
	public void destroy() {
	    try
	    {
	        if (is != null)
		    is.close();
	    }
	    catch(IOException e)
	    {
		e.printStackTrace();
	    }
	}

	/*
	 * A utility function since a BufferedInputStream has no readline().
	 */
	private byte[] buf = new byte[1024];
	
	private String readLine(BufferedInputStream input) {
	    int		index = 0;
	    int		ch;

	    try
	    {
		while ((ch = input.read()) != -1) {
		    buf[index++] = (byte) ch;
		    if (ch == '\n')
			break;
		}
	    }
	    catch(IOException e)
	    {
		e.printStackTrace();
	    }
	    if (index == 0)
		return "";
	    return new String(buf, 0, index);
	}
    }
    
    public static void main(String argv[]) {
        final WebCamApplet webcamapplet = new WebCamApplet();
	try
	{
	    webcamapplet.init();
	    webcamapplet.is = System.in;
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	
        Frame f = new Frame("WebCamApplet");
	
        f.add(webcamapplet);
        f.pack();
        f.setSize(new Dimension(640,480));
        f.show();
        webcamapplet.start();
    }
}
