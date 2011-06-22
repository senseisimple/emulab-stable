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
	import protogeni.resources.Slice;
	
	/**
	 * Used to see if a slice exists before trying to create a new one.  Uses the ProtoGENI API.
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliceRemove extends Request
	{
		private var slice:Slice;
		
		public function RequestSliceRemove(s:Slice):void
		{
			super("SliceRemove",
				"Remove slice named " + s.hrn,
				CommunicationUtil.remove);
			slice = s;
			
			// Build up the args
			op.addField("credential", Main.geniHandler.CurrentUser.Credential);
			op.addField("hrn", slice.urn.full);
			op.addField("type", "Slice");
			op.setExactUrl(Main.geniHandler.CurrentUser.authority.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var newRequest:Request = null;
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS
				|| code == CommunicationUtil.GENIRESPONSE_SEARCHFAILED)
			{
				newRequest = new RequestSliceRegister(slice);
			}
			else
			{
				Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return newRequest;
		}
	}
}
