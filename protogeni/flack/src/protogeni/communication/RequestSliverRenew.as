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
	import mx.controls.Alert;
	
	import protogeni.DateUtil;
	import protogeni.GeniEvent;
	import protogeni.resources.Slice;
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
		public var newExpires:Date;
		
		public function RequestSliverRenew(newSliver:Sliver, newExpirationDate:Date):void
		{
			super("SliverRenew",
				"Renewing sliver on " + newSliver.manager.Hrn + " on slice named " + newSliver.slice.hrn,
				CommunicationUtil.renewSliver,
				true,
				true);
			sliver = newSliver;
			newExpires = newExpirationDate;
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			op.addField("expiration", DateUtil.toRFC3339(newExpirationDate));
			op.addField("credentials", [sliver.slice.credential]);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				sliver.expires = newExpires;
				
				var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn.full);
				if(old != null) {
					for each(var oldSliver:Sliver in old.slivers.collection) {
						if(oldSliver.manager == sliver.manager) {
							oldSliver.expires = sliver.expires;
							break
						}
					}
				}
			} else
			{
				Alert.show("RenewSliver didn't work on " + sliver.manager.Hrn + ", most likely due to the manager now allowing slivers to be renewed further than a certain time in the future.  Either try a smaller increment of time or try later.",
				"Sliver not renewed");
			}
			
			return null;
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
		}
	}
}
