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
	import protogeni.resources.Key;

	/**
	 * Gets the user's keys which can be used to put keys onto allocated resources later. Uses the ProtoGENI API.
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestGetKeys extends Request
	{
		public function RequestGetKeys():void
		{
			super("GetKeys",
				"Getting the ssh credential",
				CommunicationUtil.getKeys);
			
			op.setUrl(Main.geniHandler.CurrentUser.authority.Url);
		}
		
		/**
		 * Called immediately before the operation is run to add variables it may not have had when added to the queue
		 * @return Operation to be run
		 * 
		 */
		override public function start():Operation
		{
			op.addField("credential",Main.geniHandler.CurrentUser.Credential);
			return op;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.geniHandler.CurrentUser.keys = new Vector.<Key>();
				for each(var keyObject:Object in response.value) {
					Main.geniHandler.CurrentUser.keys.push(new Key(keyObject.key, keyObject.type));
				}
				
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
