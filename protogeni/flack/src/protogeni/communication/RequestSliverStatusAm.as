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
	import protogeni.resources.IdnUrn;
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualNode;
	
	/**
	 * Gets the sliver status using the GENI AM API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverStatusAm extends Request
	{
		private var sliver:Sliver;
		
		public function RequestSliverStatusAm(newSliver:Sliver):void
		{
			super("SliverStatusAM",
				"Getting the sliver status on " + newSliver.manager.Hrn + " on slice named " + newSliver.slice.hrn,
				CommunicationUtil.sliverStatusAm,
				true);
			ignoreReturnCode = true;
			sliver = newSliver;
			
			// Build up the args
			op.pushField(sliver.slice.urn.full);
			op.pushField([sliver.slice.credential]);
			op.setExactUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			try
			{
				sliver.status = response.geni_status;
				sliver.urn = new IdnUrn(response.geni_urn);
				for each(var nodeObject:Object in response.geni_resources)
				{
					var vn:VirtualNode = sliver.nodes.getBySliverUrn(nodeObject.geni_urn);
					if(vn == null)
						vn = sliver.nodes.getByComponentUrn(nodeObject.geni_urn);
					if(vn != null)
					{
						vn.status = nodeObject.geni_status;
						vn.error = nodeObject.geni_error;
					}
				}
				Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			}
			catch(e:Error)
			{
				// do nothing
			}
			
			return null;
		}
	}
}
