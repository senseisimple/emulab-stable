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
	
	/**
	 * Gets the user's credential from their slice authority using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestGetCredential extends Request
	{
		public function RequestGetCredential():void
		{
			super("GetCredential",
				"Getting the basic user credential",
				CommunicationUtil.getCredential);
		}
		
		override public function start():Operation {
			op.setUrl(Main.geniHandler.CurrentUser.authority.Url);
			return op;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.geniHandler.CurrentUser.userCredential = String(response.value);
				var cred:XML = new XML(response.value);
				Main.geniHandler.CurrentUser.urn = new IdnUrn(cred.credential.owner_urn);
				Main.geniDispatcher.dispatchUserChanged();
			}
			else
			{
				Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return null;
		}
	}
}
