/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */

package
{
  public class Geni
  {
    private static var sa : String = "sa";
    private static var cm : String = null;
    private static var ses : String = "ses";
    private static var ch : String = "ch";

    private static var cmv2 : String = "cmv2";

//    public static var defaultAuthority : String = "jonlab.testbed.emulab.net";
    public static var defaultAuthority : String = "emulab.net";

//    public static var defaultHost : String = "myboss.jonlab.testbed.emulab.net";
    public static var defaultHost : String = "boss.emulab.net";

    public static var sesUrl : String = "https://myboss.emulab.geni.emulab.net/protogeni/xmlrpc/";
//"https://boss.emulab.net:443/protogeni/xmlrpc/";
    public static var chUrl : String = "https://boss.emulab.net/protogeni/xmlrpc/";

    public static var getCredential : Array = new Array(sa, "GetCredential");
    public static var getKeys : Array = new Array(sa, "GetKeys");
    public static var resolve : Array = new Array(sa, "Resolve");
    public static var remove : Array = new Array(sa, "Remove");
    public static var register : Array = new Array(sa, "Register");

    public static var discoverResources : Array = new Array(cm, "DiscoverResources");
    public static var getTicket : Array = new Array(cm, "GetTicket");
    public static var updateTicket : Array = new Array(cm, "UpdateTicket");
    public static var redeemTicket : Array = new Array(cm, "RedeemTicket");
    public static var releaseTicket : Array = new Array(cm, "ReleaseTicket");
    public static var deleteSliver : Array = new Array(cm, "DeleteSliver");
    public static var startSliver : Array = new Array(cm, "StartSliver");
    public static var updateSliver : Array = new Array(cm, "UpdateSliver");
    public static var resolveNode : Array = new Array(cm, "Resolve");
    public static var sliceStatus : Array = new Array(cm, "SliceStatus");
    public static var getSliver : Array = new Array(cm, "GetSliver");
    public static var sliverStatus : Array = new Array(cm, "SliverStatus");
    public static var sliverTicket : Array = new Array(cm, "SliverTicket");

    public static var discoverResourcesv2 : Array = new Array(cmv2, "DiscoverResources");

    public static var map : Array = new Array(ses, "Map");

    public static var listComponents : Array = new Array(ch, "ListComponents");
  }
}
