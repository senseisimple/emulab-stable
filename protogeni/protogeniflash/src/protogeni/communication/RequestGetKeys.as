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
	
	public class RequestGetKeys extends Request
	{
		public function RequestGetKeys() : void
		{
			super("GetKeys", "Getting the ssh credential", CommunicationUtil.getKeys);
		}
		
		override public function start():Operation
		{
			op.addField("credential",Main.protogeniHandler.CurrentUser.credential);
			return op;
		}
		
		override public function complete(code : Number, response : Object) : *
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.protogeniHandler.CurrentUser.keys = response.value;
				Main.protogeniHandler.dispatchUserChanged();
			}
			else
			{
				Main.protogeniHandler.rpcHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return null;
		}
	}
}
