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

package protogeni.communication
{
	import protogeni.Util;
	import protogeni.resources.AggregateManager;

  public class RequestGetVersion extends Request
  {
    public function RequestGetVersion(newAm:AggregateManager) : void
    {
      super("GetVersion (" + Util.shortenString(newAm.Url, 15) + ")", "Getting the version of the aggregate manager for " + newAm.Hrn, CommunicationUtil.getVersionAm, true, true, true);
	  ignoreReturnCode = true;
	  am = newAm;
	  op.setUrl(am.Url);
    }

	// Should return Request or RequestQueueNode
	override public function complete(code : Number, response : Object) : *
	{
		var r:Request = null;
		try
		{
			am.Version = response.geni_api;
			r = new RequestListResources(am);
			r.forceNext = true;
		}
		catch(e:Error)
		{
		}
		
		return r;
	}
	
	private var am:AggregateManager;
  }
}
