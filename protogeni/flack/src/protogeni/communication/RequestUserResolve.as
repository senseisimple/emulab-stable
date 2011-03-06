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
	import protogeni.resources.Slice;
	
	public final class RequestUserResolve extends Request
	{
		public function RequestUserResolve():void
		{
			super("UserResolve",
				"Resolve user",
				CommunicationUtil.resolve);
			op.addField("type", "User");
			op.setUrl(Main.geniHandler.CurrentUser.authority.Url);
		}
		
		override public function start():Operation
		{
			op.addField("credential", Main.geniHandler.CurrentUser.credential);
			op.addField("hrn", Main.geniHandler.CurrentUser.urn.full);
			return op;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var newCalls:RequestQueue = new RequestQueue();
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.geniHandler.CurrentUser.uid = response.value.uid;
				Main.geniHandler.CurrentUser.hrn = response.value.hrn;
				Main.geniHandler.CurrentUser.email = response.value.email;
				Main.geniHandler.CurrentUser.name = response.value.name;
				
				var slices:Array = response.value.slices;
				if(slices != null && slices.length > 0) {
					for each(var sliceUrn:String in slices)
					{
						var userSlice:Slice = new Slice();
						userSlice.urn = new IdnUrn(sliceUrn);
						Main.geniHandler.CurrentUser.slices.add(userSlice);
						newCalls.push(new RequestSliceResolve(userSlice));
					}
				}
				
				Main.geniDispatcher.dispatchUserChanged();
				Main.geniDispatcher.dispatchSlicesChanged();
			}
			else
			{
				Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return newCalls.head;
		}
	}
}
