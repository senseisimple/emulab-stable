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
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualNode;
	
  public class RequestSliverStatusAm extends Request
  {
    public function RequestSliverStatusAm(s:Sliver) : void
    {
		super("SliverStatus", "Getting the sliver status on " + s.manager.Hrn + " on slice named " + s.slice.hrn, CommunicationUtil.sliverStatusAm, true);
		ignoreReturnCode = true;
		sliver = s;
		op.pushField(sliver.slice.urn);
		op.pushField([sliver.slice.credential]);
		op.setUrl(sliver.manager.Url);
    }

	override public function complete(code : Number, response : Object) : *
	{
		try
		{
			sliver.status = response.geni_status;
			sliver.urn = response.geni_urn;
			for each(var nodeObject:Object in response.geni_resources)
			{
				var vn:VirtualNode = sliver.getVirtualNodeFor(sliver.manager.Nodes.GetByUrn(nodeObject.geni_urn));
				if(vn != null)
				{
					vn.status = nodeObject.geni_status;
					vn.error = nodeObject.geni_error;
				}
			}
			Main.geniHandler.dispatchSliceChanged(sliver.slice);
		}
		catch(e:Error)
		{
			// do nothing
		}
		
		return null;
	}
	
	private var sliver:Sliver;
  }
}
