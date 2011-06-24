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
	import protogeni.DateUtil;
	import protogeni.GeniEvent;
	import protogeni.resources.Sliver;
	
	/**
	 * Gets the manifest for a sliver using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverRenew extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverRenew(newSliver:Sliver, newExpirationDate:Date):void
		{
			super("SliverRenew",
				"Renewing sliver on " + newSliver.manager.Hrn + " on slice named " + newSliver.slice.hrn,
				CommunicationUtil.renewSliver,
				true,
				true);
			sliver = newSliver;
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			op.addField("expiration", DateUtil.toW3CDTF(newExpirationDate));
			op.addField("credentials", [sliver.slice.credential]);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				// XXX what now???
			}
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			return null;
		}
	}
}
