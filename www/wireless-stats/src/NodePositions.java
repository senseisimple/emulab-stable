/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * NodePositions.java
 *
 * Created on July 19, 2005, 8:26 PM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

import java.io.*;
import java.util.*;
import java.util.regex.*;

/**
 *
 * @author david
 */
public class NodePositions implements NodePosition {
    
    private Hashtable positions;
    
    /** Creates a new instance of NodePositions */
    protected NodePositions(Hashtable positions) {
        this.positions = positions;
    }
    
    public NodePositions parseDB() {
        return null;
    }
    
    private static NodePositions parseReader(BufferedReader br) 
        throws IOException, java.text.ParseException  {
        
        Hashtable pH = new Hashtable();
        
        try {
            //br = new BufferedReader(new FileReader(f));
            
            String line = null;
            
            while ((line = br.readLine()) != null) {
                // parse line: "mote101   350   708   0.2   4"
                // where line corresponds to "<nodename> <x> <y> <z> <floor>""
                Pattern posit = Pattern.compile("([a-zA-Z0-9\\-_]+)\\s+(.+)\\s+(.+)\\s+(.+)\\s+(.+)");
                Matcher positM = posit.matcher(line);
                
                if (positM.matches()) {
                    float p[] = new float[3];
                    int floor = -1;;
                    try {
                        p[0] = 0.0f;
                        p[0] = Float.parseFloat(positM.group(2));
                    }
                    catch (Exception ex) { }
                    try {
                        p[1] = 0.0f;
                        p[1] = Float.parseFloat(positM.group(3));
                    }
                    catch (Exception ex) { }
                    try {
                        p[2] = 0.0f;
                        p[2] = Float.parseFloat(positM.group(4));
                    }
                    catch (Exception ex) { }
                    try {
                        floor = Integer.parseInt(positM.group(5));
                    }
                    catch (Exception ex) { }
                    
                    pH.put(positM.group(1),new Position(positM.group(1),p[0],p[1],p[2],floor));
                    
                    System.err.println(""+positM.group(1)+" ("+p[0]+","+p[1]+","+p[2]+"; "+floor+")");
                }
                else {
                    System.err.println("positline '"+line+"' did not match!");
                }
            }
            
        }
        catch (IOException e) {
            //throw new Exception("file read failed");
            throw e;
        }
        
        return new NodePositions(pH);
    }
    
    public java.util.Enumeration getNodeList() {
        return this.positions.keys();
    }
    
    public static NodePositions parseInputStream(InputStream is) 
        throws IOException, java.text.ParseException, Exception {
        
        BufferedReader br = null;
        NodePositions retval = null;
        
        br = new BufferedReader(new InputStreamReader(is));
        retval = parseReader(br);
        
        return retval;
    }
    
    public static NodePositions parseFile(String filename) 
        throws java.io.IOException, java.text.ParseException,
        java.lang.IllegalArgumentException, Exception {
        File f = null;
        BufferedReader br = null;

        if (filename != null) {
            f = new File(filename);
            if (!f.exists() || !f.canRead()) {
                 throw new java.io.IOException("Couldn't open and/or read " + 
                                               "file "+filename);
            }
        }
        else {
            throw new java.lang.IllegalArgumentException("filename null");
        }
        
        br = new BufferedReader(new FileReader(f));
        return parseReader(br);
    }
    
    public Position getPosition(String nodeName) {
        return (Position)positions.get(nodeName);
    }
    
    public java.awt.Point getPoint(String nodeName) {
        //System.out.println("looking for node '"+nodeName+"'");
        Position p = (Position)(positions.get(nodeName));
        if (p != null) {
            return new java.awt.Point((int)p.x,(int)p.y);
        }
        else {
            return null;
        }
    }
    
    public static void main(String args[]) throws Exception {
        parseFile(args[0]);
    }
    
}
