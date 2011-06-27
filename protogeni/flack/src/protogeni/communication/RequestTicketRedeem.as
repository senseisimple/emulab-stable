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
	import com.mattism.http.xmlrpc.MethodFault;
	
	import flash.events.ErrorEvent;
	
	import protogeni.resources.Key;
	import protogeni.resources.Sliver;
	
	public final class RequestTicketRedeem extends Request
	{
		public var sliver:Sliver;
		
		/**
		 * Redeems a ticket previously given to the user using the ProtoGENI API
		 * 
		 * @param s
		 * 
		 */
		public function RequestTicketRedeem(newSliver:Sliver):void
		{
			super("TicketRedeem",
				"Updating ticket for sliver on " + newSliver.manager.Hrn + " for slice named " + newSliver.slice.hrn,
				CommunicationUtil.redeemTicket);
			sliver = newSliver;
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			op.addField("credentials", [sliver.credential]);
			op.addField("ticket", sliver.ticket);
			var keys:Array = [];
			for each(var key:Key in sliver.slice.creator.keys) {
				keys.push({type:key.type, key:key.value});
			}
			op.addField("keys", keys);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				sliver.credential = response.value[0];
				sliver.created = true;
				
				sliver.rspec = new XML(response.value[1]);
				sliver.parseRspec();
				
				return new RequestSliverStart(sliver);
			}
			else
			{
				return new RequestTicketRelease(sliver);
			}
			
			return null;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			return new RequestTicketRelease(sliver);
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
		}
	}
}
