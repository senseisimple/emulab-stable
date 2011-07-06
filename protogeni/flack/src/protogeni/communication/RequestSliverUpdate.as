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
	
	public final class RequestSliverUpdate extends Request
	{
		public var sliver:Sliver;
		public var rspec:String = "";
		
		/**
		 * Creates a redeemable ticket for a sliver with a different configuration using the ProtoGENI API
		 * 
		 * @param newSliver
		 * 
		 */
		public function RequestSliverUpdate(newSliver:Sliver, useRspec:XML = null):void
		{
			super("SliverUpdate",
				"Updating sliver on " + newSliver.manager.Hrn + " for slice named " + newSliver.slice.hrn,
				CommunicationUtil.updateSliver);
			sliver = newSliver;
			sliver.created = false;
			sliver.staged = false;
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			// Build up the args
			op.addField("sliver_urn", sliver.urn.full);
			if(useRspec != null)
				rspec = useRspec.toXMLString()
			else
				rspec = sliver.getRequestRspec(false).toXMLString();
			op.addField("rspec", rspec);
			op.addField("credentials", [sliver.slice.credential]);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				sliver.ticket = response.value;
				return new RequestTicketRedeem(sliver);
			}
			if (code == CommunicationUtil.GENIRESPONSE_REFUSED)
			{
				//
			}
			else
			{
				// do nothing
			}
			
			return null;
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
		}
		
		override public function getSent():String {
			return op.getSent() + "\n\n********RSPEC********\n\n" + rspec;
		}
	}
}
