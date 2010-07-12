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
  import flash.events.ErrorEvent;
  import flash.events.SecurityErrorEvent;
  import protogeni.resources.Slice;

  public class RequestUserResolve extends Request
  {
    public function RequestUserResolve() : void
    {
		super("UserResolve", "Resolve user", CommunicationUtil.resolve);
		op.addField("type", "User");
		op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
    }
	
	override public function start():Operation
	{
		op.addField("credential", Main.protogeniHandler.CurrentUser.credential);
		op.addField("hrn", Main.protogeniHandler.CurrentUser.urn);
		return op;
	}
	
	override public function complete(code : Number, response : Object) : *
	{
		var newCalls:RequestQueue = new RequestQueue();
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			Main.protogeniHandler.CurrentUser.uid = response.value.uid;
			Main.protogeniHandler.CurrentUser.hrn = response.value.hrn;
			Main.protogeniHandler.CurrentUser.uuid = response.value.uuid;
			Main.protogeniHandler.CurrentUser.email = response.value.email;
			Main.protogeniHandler.CurrentUser.name = response.value.name;
			Main.protogeniHandler.dispatchUserChanged();
			
			var sliceHrns:Array = response.value.slices;
			if(sliceHrns != null && sliceHrns.length > 0) {
				for each(var sliceHrn:String in sliceHrns)
				{
					var userSlice:Slice = new Slice();
					userSlice.hrn = sliceHrn;
					Main.protogeniHandler.CurrentUser.slices.add(userSlice);
					newCalls.push(new RequestSliceResolve(userSlice));
				}
			}
			else
				Main.protogeniHandler.mapHandler.drawAll();
		}
		else
		{
			Main.protogeniHandler.rpcHandler.codeFailure(name, "Recieved GENI response other than success");
		}
		
		return newCalls.head;
	}
  }
}
