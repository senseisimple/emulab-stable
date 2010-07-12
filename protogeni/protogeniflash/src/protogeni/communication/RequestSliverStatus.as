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
	
  public class RequestSliverStatus extends Request
  {
    public function RequestSliverStatus(s:Sliver) : void
    {
		super("SliverStatus", "Getting the sliver status on " + s.componentManager.Hrn + " on slice named " + s.slice.hrn, CommunicationUtil.sliverStatus, true);
		sliver = s;
		op.addField("slice_urn", sliver.slice.urn);
		op.addField("credentials", new Array(sliver.slice.credential));
		op.setExactUrl(sliver.componentManager.Url);
    }

	override public function complete(code : Number, response : Object) : *
	{
		var newCall:Request = new Request();
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			sliver.status = response.value.status;
			sliver.state = response.value.state;
			newCall = new RequestSliverResolve(sliver);
		}
		else
		{
			// do nothing
		}
		
		return newCall;
	}
	
	private var sliver:Sliver;
  }
}
