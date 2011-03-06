/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
	import protogeni.resources.Sliver;

  public final class RequestSliverRestart extends Request
  {
    public function RequestSliverRestart(s:Sliver) : void
    {
		super("SliverRestart", "Restarting sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn, CommunicationUtil.restartSliver);
		sliver = s;
		op.addField("slice_urn", sliver.slice.urn.full);
		op.addField("credentials", new Array(sliver.slice.credential));
		op.setUrl(sliver.manager.Url);
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			return new RequestSliverStatus(sliver);
		}
		else
		{
			// do nothing
		}
		
		return null;
	}
	
	public var sliver:Sliver;
  }
}
