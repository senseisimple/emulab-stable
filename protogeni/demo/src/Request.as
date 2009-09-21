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
  public class Request
  {
    public function Request(newServer : String) : void
    {
      op = new Operation(null);
      opName = "";
      opServer = newServer;
    }

    public function cleanup() : void
    {
      op.cleanup();
    }

    public function getSendXml() : String
    {
      return op.getSendXml();
    }

    public function getResponseXml() : String
    {
      return op.getResponseXml();
    }

    public function getOpName() : String
    {
      return opName + " (" + opServer + ")";
    }

    public function getUrl() : String
    {
      return op.getUrl();
    }

    public function start(credential : Credential) : Operation
    {
      return op;
    }

    public function complete(code : Number, response : Object,
                             credential : Credential) : Request
    {
      return null;
    }

    public function fail() : Request
    {
      return null;
    }

    protected var op : Operation;
    protected var opName : String;
    protected var opServer : String;

    public static var IMPOTENT : int = 0;
  }
}
