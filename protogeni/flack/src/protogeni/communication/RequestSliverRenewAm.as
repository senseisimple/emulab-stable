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
	import protogeni.resources.IdnUrn;
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualNode;
	
	/**
	 * Gets the sliver status using the GENI AM API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverRenewAm extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverRenewAm(newSliver:Sliver, newExpirationDate:Date):void
		{
			super("SliverRenewAM",
				"Renewing the sliver on " + newSliver.manager.Hrn + " on slice named " + newSliver.slice.hrn,
				CommunicationUtil.sliverStatusAm,
				true,
				true);
			ignoreReturnCode = true;
			sliver = newSliver;
			
			// Build up the args
			op.pushField(sliver.slice.urn.full);
			op.pushField([sliver.slice.credential]);
			op.pushField(DateUtil.toW3CDTF(newExpirationDate));
			op.setExactUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			// did it work???
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			return null;
		}
	}
}
