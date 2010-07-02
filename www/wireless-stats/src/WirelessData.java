/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

public interface WirelessData {
    
    public String[] getNodes();
    //public LinkStats[] getSendStats(String srcNode);
    //public LinkStats[] getRecvStats(String recvNode);
    public int[] getPowerLevels();

    // otherwise you have to use these... ick!
    public LinkStats[] getSendStatsAtPowerLevel(String srcNode,int powerLevel);
    public LinkStats[] getRecvStatsAtPowerLevel(String recvNode,int powerLevel);
    public LinkStats[] getAllStatsAtPowerLevel(int powerLevel);
    
}
