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
	import protogeni.resources.VirtualNode;
	
	/**
	 * Gets the sliver status using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverStatus extends Request
	{
		private var sliver:Sliver;
		
		public function RequestSliverStatus(newSliver:Sliver):void
		{
			super("SliverStatus",
				"Getting the sliver status on " + newSliver.manager.Hrn + " on slice named " + newSliver.slice.hrn,
				CommunicationUtil.sliverStatus,
				true);
			sliver = newSliver;
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			op.addField("credentials", new Array(sliver.slice.credential));
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
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
	}
}
