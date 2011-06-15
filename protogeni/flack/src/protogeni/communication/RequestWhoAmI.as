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
	import protogeni.StringUtil;
	import protogeni.Util;
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.Slice;
	import protogeni.resources.SliceAuthority;
	import protogeni.resources.Sliver;
	
	/**
	 * Gets the list of component managers from the clearinghouse using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestWhoAmI extends Request
	{
		public function RequestWhoAmI():void
		{
			super("WhoAmI",
				"Finding out who I am",
				CommunicationUtil.whoAmI);
		}
		
		// Should return Request or RequestQueueNode
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.geniHandler.CurrentUser.urn = new IdnUrn(response.value.urn);
				for each(var sa:SliceAuthority in Main.geniHandler.GeniAuthorities.source) {
					if(sa.Urn.full == response.value.sa_urn) {
						Main.geniHandler.CurrentUser.authority = sa;
						break;
					}
				}
			}
			else
			{
			}
			
			Main.geniHandler.requestHandler.startAuthenticatedInitiationSequence();
			
			return null;
		}
	}
}
