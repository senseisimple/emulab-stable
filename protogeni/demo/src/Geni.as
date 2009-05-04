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
    static var ch : String = "sa";
    static var cm : String = "cm";

    public static var getCredential = new Array(ch, "GetCredential");
    public static var getKeys = new Array(ch, "GetKeys");
    public static var resolve = new Array(ch, "Resolve");
    public static var remove = new Array(ch, "Remove");
    public static var register = new Array(ch, "Register");

    public static var discoverResources = new Array(cm, "DiscoverResources");
    public static var getTicket = new Array(cm, "GetTicket");
    public static var redeemTicket = new Array(cm, "RedeemTicket");
    public static var releaseTicket = new Array(cm, "ReleaseTicket");
    public static var deleteSliver = new Array(cm, "DeleteSliver");
    public static var startSliver = new Array(cm, "StartSliver");
    public static var updateSliver = new Array(cm, "UpdateSliver");
    public static var resolveNode = new Array(cm, "Resolve");
  }
}
