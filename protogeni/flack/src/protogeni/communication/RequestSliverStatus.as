﻿/* GENIPUBLIC-COPYRIGHT
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
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualNode;
	
  public class RequestSliverStatus extends Request
  {
    public function RequestSliverStatus(s:Sliver) : void
    {
		super("SliverStatus", "Getting the sliver status on " + s.manager.Hrn + " on slice named " + s.slice.hrn, CommunicationUtil.sliverStatus, true);
		sliver = s;
		op.addField("slice_urn", sliver.slice.urn);
		op.addField("credentials", new Array(sliver.slice.credential));
		op.setUrl(sliver.manager.Url);
    }

	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			sliver.status = response.value.status;
			sliver.state = response.value.state;
			for each(var nodeObject:Object in response.value.details)
			{
				var vn:VirtualNode = sliver.getVirtualNodeFor(sliver.manager.Nodes.GetByUrn(nodeObject.component_urn));
				if(vn != null)
				{
					vn.status = nodeObject.status;
					vn.state = nodeObject.state;
					vn.error = nodeObject.error;
				}
			}
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			Main.geniDispatcher.dispatchSlicesChanged();
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
