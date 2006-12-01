/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * GenericWirelessData.java
 *
 * Created on July 6, 2006, 7:45 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

import java.util.*;

/**
 *
 * @author david
 */
public interface GenericWirelessData {
    
    public String[] getNodes();
    //public LinkStats[] getSendStats(String srcNode);
    //public LinkStats[] getRecvStats(String recvNode);
    public String[] getIndices();
    public Vector getIndexValues(String index);
    
    public Vector getProperties();
    
    public Object getMinPropertyValue(String property);
    public Object getMaxPropertyValue(String property);

    // otherwise you have to use these... ick!
    public GenericLinkStats[] getSendStatsAtIndexValues(String srcNode,String[] indexValues);
    public GenericLinkStats[] getRecvStatsAtIndexValues(String recvNode,String[] indexValues);
    public GenericLinkStats[] getAllStatsAtIndexValues(String[] indexValues);
    
}
