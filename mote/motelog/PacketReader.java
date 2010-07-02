/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.io.*;
import java.util.*;

public final class PacketReader {


    /*
     * creates a class that decomposes an InputStream into tinyos
     * packets.  IT DOES NOT REPLY to any packets, since it is meant
     * to only be used by our logger.  All the logger does is snoop
     * packets... we want to let any applications respond as necessary.
     * As it turns out, when we snoop the Elab component-generated packets,
     * we'll issue any acks then -- or let some special "elab listener" do it.
     * 
     */

    /*
     * Here's the relevant protocol info from net.tinyos.packet.Packetizer:
     * 
     * Protocol inspired by, but not identical to, RFC 1663.
     * There is currently no protocol establishment phase, and a single
     * byte ("packet type") to identify the kind/target/etc of each packet.
     *
     * The protocol is really, really not aiming for high performance.
     * 
     * There is however a hook for future extensions: implementations are
     * required to answer all unknown packet types with a P_UNKNOWN packet.
     *
     * To summarise the protocol:
     * - the two sides (A & B) are connected by a (potentially unreliable)
     *   byte stream
     * - the two sides exchange packets framed by 0x7e (SYNC_BYTE) bytes
     * - each packet has the form
     *     <packet type> <data bytes 1..n> <16-bit crc>
     *   where the crc (see net.tinyos.util.Crc) covers the packet type
     *   and bytes 1..n
     * - bytes can be escaped by preceding them with 0x7d and their
     *   value xored with 0x20; 0x7d and 0x7e bytes must be escaped, 
     *   0x00 - 0x1f and 0x80-0x9f may be optionally escaped
     * - There are currently 5 packet types:
     *   P_PACKET_NO_ACK: A user-packet, with no ack required
     *   P_PACKET_ACK: A user-packet with a prefix byte, ack required. 
     *     The receiver must send a P_ACK packet with the prefix byte
     *     as its contents.
     *   P_ACK: ack for a previous P_PACKET_ACK packet
     *   P_UNKNOWN: unknown packet type received. On reception of an
     *     unknown packet type, the receicer must send a P_UNKNOWN packet,
     *     the first byte must be the unknown packet type.
     * - Packets that are greater than a (private) MTU are silently dropped.
     */

    /* see http://www.tinyos.net/tinyos-1.x/doc/serialcomm/description.html */

    final static int SYNC_BYTE = 0x7e; 
    final static int ESCAPE_BYTE = 0x7d;
    final static int MTU = 256;
    // don't need this cause we're not responding.
    //final static int ACK_TIMEOUT = 1000; // in milliseconds

    final static int P_ACK = 64;
    final static int P_PACKET_ACK = 65;
    final static int P_PACKET_NO_ACK = 66;
    final static int P_UNKNOWN = 255;

    private InputStream in;
    private String vNodeName;

    public PacketReader(String vNodeName,InputStream in) {
	this.vNodeName = vNodeName;
	this.in = in;
    }

    private void debug(int level,String msg) {
	MoteLogger.globalDebug(level,"PacketReader: " + msg);
    }

    private void error(String msg) {
	MoteLogger.globalError("PacketReader: " + msg);
    }

    // this method is meant to be called repeatedly, at a high rate.
    // if "arrangements" in the caller (like synchronization with a queue)
    // make fast, repetitious calls impossible, the timestamps will get off.
    // if this is a concern, this class should be threaded itself and send 
    // notifications to the thread that does the logging, or implement a 
    // queue for that application.  That's the right way to do it, but
    // we're not dealing with high throughput links here!  So speed is
    // the better half of quality here... or something like that.
    public LogPacket readPacket() throws IOException {
	// just implement the simple packet protocol, reading
	// byte by byte for now -- ick!
	// obviously, this method is NOT thread-safe.

	debug(3,"trying to read a packet"); 

	byte[] buf = new byte[MTU];
	int currentOffset = 0;
	boolean isEscaped = false;
	boolean sync = false;

	while (true) {
	    if (currentOffset >= buf.length) {
		// mtu reached, toss it.
		for (int i = 0; i < buf.length; ++i) {
		    buf[i] = 0;
		}
		currentOffset = 0;
		sync = false;
		isEscaped = false;
	    }
	    
	    // read byte -- let the exception fly through to the caller
	    int b = (int)in.read();
	    debug(5,"read byte: "+b);

	    if (b == ESCAPE_BYTE) {
		isEscaped = true;
	    }
	    else if (b == SYNC_BYTE) {
		if (isEscaped && sync) {
		    // ex-xor it
		    b ^= 0x20;
		    buf[currentOffset++] = (byte)b;
		    isEscaped = false;
		}
		else if (isEscaped) {
		    // we're trying to sync via an escaped data byte, bad us
		    // i.e., we're coming in in the middle of a packet.
		}
		else if (sync) {
		    // end of packet
		    debug(3,"saw end sync byte");
		    break;
		}
		else {
		    debug(3,"saw first sync byte");
		    sync = true;
		}
	    }
	    else {
		// normal data byte:
		if (isEscaped) {
		    b ^= 0x20;
		    isEscaped = false;
		}

		buf[currentOffset++] = (byte)b;
	    }
	}

	// we've got a packet, with only the SYNC_BYTEs and ESCAPE_BYTEs 
	// stripped out.  Now we have to check the packet type and the crc.
	
	int crc = 0;
	int packetType = 0;
	byte[] retval;
	Date t = new Date();
	LogPacket lp = null;

	crc = (0xff & buf[buf.length-1]) << 8;
	crc |= 0xff & buf[buf.length-2];
	packetType = buf[0];

	if (buf[0] == P_PACKET_NO_ACK) {
	    // no "prefix" byte
	    retval = new byte[currentOffset-3];
	    System.arraycopy(buf,1,retval,0,retval.length);
	    
	    debug(2,"recv no ack pkt; length="+retval.length);

	    return new LogPacket(vNodeName,t,retval,packetType,crc);
	}
	else if (buf[0] == P_PACKET_ACK) {
	    retval = new byte[buf.length-4];
	    System.arraycopy(buf,2,retval,0,retval.length);

	    debug(2,"recv ack pkt; length="+retval.length);
	    
	    return new LogPacket(vNodeName,t,retval,packetType,crc);
	}
	else if (buf[0] == P_ACK || buf[0] == P_UNKNOWN) {
	    // do nothing for now; this is only sent by receiver on
	    // receipt of an unknown packet type anyway
	    ;

	    debug(2,"recv ack/unknown pkt!");
	}
	else {
	    // XXX: might want to log these in the future...

	    debug(2,"recv TOTALLY unknown pkt!");
	}

	return null;
    }

}

