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
	import protogeni.Util;
	import protogeni.display.DisplayUtil;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	
	/**
	 * Creates a new slice and gets its credential using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliceRenew extends Request
	{
		public var slice:Slice;
		private var renewSlivers:Boolean;
		private var expirationDate:Date;
		
		public function RequestSliceRenew(s:Slice, newExpirationDate:Date, shouldRenewSlivers:Boolean = false):void
		{
			super("SliceRenew",
				"Renewing slice named " + s.hrn,
				CommunicationUtil.renewSlice,
				false,
				true);
			slice = s;
			renewSlivers = shouldRenewSlivers;
			expirationDate = newExpirationDate;
			
			// Build up the args
			op.addField("credential", slice.credential);
			op.addField("expiration", DateUtil.toW3CDTF(newExpirationDate));
			op.setExactUrl(Main.geniHandler.CurrentUser.authority.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var newRequest:RequestQueueNode = null;
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				slice.credential = String(response.value);
				
				var cred:XML = new XML(slice.credential);
				slice.expires = Util.parseProtogeniDate(cred.credential.expires);
				
				if(renewSlivers) {
					var newRequest = new RequestQueueNode();
					var currentRequest = newRequest;
					for each(var sliver:Sliver in slice.slivers.collection) {
						var nextRequest:RequestQueueNode = new RequestQueueNode();
						if(sliver.manager.isAm)
							nextRequest.item = new RequestSliverRenewAm(sliver, expirationDate);
						else
							nextRequest.item = new RequestSliverRenew(sliver, expirationDate);
						currentRequest.next = nextRequest;
						currentRequest = newRequest;
					}
				}
			}
			
			return newRequest;
		}
	}
}
