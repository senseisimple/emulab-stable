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
	import protogeni.Util;
	import protogeni.resources.GeniManager;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.Sliver;
	
	/**
	 * Gets some info for the sliver like the sliver credential using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverGet extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverGet(s:Sliver):void
		{
			super("SliverGet", "Getting the sliver on " + s.manager.Hrn + " on slice named " + s.slice.hrn,
				CommunicationUtil.getSliver,
				true,
				true);
			sliver = s;
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			op.addField("credentials", [sliver.slice.credential]);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function start():Operation {
			if(sliver.manager.Status == GeniManager.STATUS_VALID)
				return op;
			else
				return null;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var newCall:Request = null;
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				sliver.credential = String(response.value);
				
				var cred:XML = new XML(response.value);
				sliver.urn = new IdnUrn(cred.credential.target_urn);
				sliver.expires = Util.parseProtogeniDate(cred.credential.expires);
				
				newCall = new RequestSliverResolve(sliver);
			}
			else if(code == CommunicationUtil.GENIRESPONSE_SEARCHFAILED)
			{
				// NO SLICE FOUND HERE
			}
			
			return newCall;
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
		}
	}
}
