
import java.util.*;
import net.tinyos.message.Message;

public class LogPacket {

    private Date timeStamp;
    private byte[] data;
    private int packetType;
    private int crc;
    private Object msg;
    private String srcVnodeName;

    public LogPacket(String srcVnodeName,Date time,byte[] data,
		     int packetType,int crc) {
	this.srcVnodeName = srcVnodeName;
	this.timeStamp = time;
	this.data = data;
	this.packetType = packetType;
	this.crc = crc;
	//this.msg = null;
    }

    public Date getTimeStamp() {
	return this.timeStamp;
    }

    public byte[] getData() {
	return this.data;
    }

    public int getLowLevelPacketType() {
	return this.packetType;
    }

    public int getLowLevelCRC() {
	return this.crc;
    }

    public String getSrcVnodeName() {
	return this.srcVnodeName;
    }

    public void setMsgObject(Object obj) {
	this.msg = obj;
    }

    public Object getMsgObject() {
	return this.msg;
    }

}
