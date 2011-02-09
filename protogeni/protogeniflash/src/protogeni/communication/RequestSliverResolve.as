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
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;

  public class RequestSliverResolve extends Request
  {
    public function RequestSliverResolve(s:Sliver) : void
    {
		super("SliverResolve", "Resolving sliver on " + s.manager.Hrn + " on slice named " + s.slice.hrn, CommunicationUtil.resolveResource, true);
		sliver = s;
		op.addField("urn", sliver.urn);
		op.addField("credentials", new Array(sliver.credential));
		op.setUrl(sliver.manager.Url);
    }

	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			sliver.rspec = new XML(response.value.manifest);
			sliver.created = true;
			sliver.parseRspec();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			return new RequestSliverStatus(sliver);
		}
		else
		{
			// do nothing
		}
		
		return null;
	}
	
	private var sliver:Sliver;
  }
}
