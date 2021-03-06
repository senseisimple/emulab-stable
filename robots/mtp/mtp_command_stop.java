/*
 * Automatically generated by jrpcgen 1.0.5 on 1/8/05 2:03 PM
 * jrpcgen is part of the "Remote Tea" ONC/RPC package for Java
 * See http://acplt.org/ks/remotetea.html for details
 */
package net.emulab;
import org.acplt.oncrpc.*;
import java.io.IOException;

public class mtp_command_stop implements XdrAble {
    public int command_id;
    public int robot_id;

    public mtp_command_stop() {
    }

    public mtp_command_stop(XdrDecodingStream xdr)
           throws OncRpcException, IOException {
        xdrDecode(xdr);
    }

    public void xdrEncode(XdrEncodingStream xdr)
           throws OncRpcException, IOException {
        xdr.xdrEncodeInt(command_id);
        xdr.xdrEncodeInt(robot_id);
    }

    public void xdrDecode(XdrDecodingStream xdr)
           throws OncRpcException, IOException {
        command_id = xdr.xdrDecodeInt();
        robot_id = xdr.xdrDecodeInt();
    }

}
// End of mtp_command_stop.java
