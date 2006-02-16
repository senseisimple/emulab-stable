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
        
        Hashtable p = new Hashtable();
        
        try {
            //br = new BufferedReader(new FileReader(f));
            
            String line = null;
            
            while ((line = br.readLine()) != null) {
                // parse line: "mote101   350   708   0.2"
                Pattern posit = Pattern.compile("([a-zA-Z0-9\\-_]+)\\s+(\\-*\\d+)\\s+(\\-*\\d+)\\s+(\\-*\\d+\\.\\d+E*\\-*\\d*)");
                Matcher positM = posit.matcher(line);
                
                if (positM.matches()) {
                    float xyz[] = new float[3];
                    
                    xyz[0] = Float.parseFloat(positM.group(2));
                    xyz[1] = Float.parseFloat(positM.group(3));
                    xyz[2] = Float.parseFloat(positM.group(4));
                    
                    p.put(positM.group(1),xyz);
                    
                    //System.out.println(""+positM.group(1)+" ("+xyz[0]+","+xyz[1]+","+xyz[2]+")");
                }
                
            }
            
        }
        catch (IOException e) {
            //throw new Exception("file read failed");
            throw e;
        }
        
        
        return new NodePositions(p);
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
    
    public float[] getPosition(String nodeName) {
        float retval[] = null;
        retval = (float[])(positions.get(nodeName));
        return retval;
    }
    
    public java.awt.Point getPoint(String nodeName) {
        //System.out.println("looking for node '"+nodeName+"'");
        float p[] = null;
        p = (float[])(positions.get(nodeName));
        if (p != null) {
            return new java.awt.Point((int)p[0],(int)p[1]);
        }
        else {
            return null;
        }
    }
    
    public static void main(String args[]) throws Exception {
        parseFile(args[0]);
    }
    
}
