
public class LinkStats {
    private String srcNode;
    private String recvNode;
    private int pktRecvCount;
    private int pktCount;
    private float pktPercent;
    private float minRSSI;
    private float maxRSSI;
    private float avgRSSI;
    private float stddevRSSI;
    
    public LinkStats(String srcNode,String recvNode,
                     int pktRecvCount,int pktCount,
                     float minRSSI,float maxRSSI,float avgRSSI,
                     float stddevRSSI) {
        this.srcNode = srcNode;
        this.recvNode = recvNode;
        this.pktRecvCount = pktRecvCount;
        this.pktCount = pktCount;
        this.pktPercent = ((float)this.pktRecvCount)/this.pktCount;
        this.minRSSI = minRSSI;
        this.maxRSSI = maxRSSI;
        this.avgRSSI = avgRSSI;
        this.stddevRSSI = stddevRSSI;
    }
    
    public String getSrcNode() {
        return srcNode;
    }
    
    public String getRecvNode() {
        return recvNode;
    }
    
    public int getPktCount() {
        return pktRecvCount;
    }
    
    public int getExpectedPktCount() {
        return pktCount;
    }
    
    public float getPktPercent() {
        return pktPercent;
    }
    
    public float getMinRSSI() {
        return minRSSI;
    }
    
    public float getMaxRSSI() {
        return maxRSSI;
    }
    
    public float getAvgRSSI() {
        return avgRSSI;
    }
    
    public float getStddevRSSI() {
        return stddevRSSI;
    } 
}