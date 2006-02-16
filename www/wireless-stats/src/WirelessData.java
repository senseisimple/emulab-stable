
/**
 *
 * This interface must be implemented by anything th
 *
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
