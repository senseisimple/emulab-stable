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
	
	import mx.collections.ArrayList;
	
	import protogeni.resources.GeniManager;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.SliceAuthority;
	
	/**
	 * Gets the list of managers for an unauthenticated user
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestListAuthoritiesPublic extends Request
	{
		public function RequestListAuthoritiesPublic():void
		{
			super("ListAuthoritiesPublic",
				"Getting the list of slice authorities",
				null);
			op.setExactUrl(Main.geniHandler.salistUrl);
			op.type = Operation.HTTP;
		}
		
		// Should return Request or RequestQueueNode
		override public function complete(code:Number, response:Object):*
		{
			var newCalls:RequestQueue = new RequestQueue();
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.geniHandler.GeniAuthorities = new ArrayList();
				var sliceAuthorityLines:Array = (response as String).split(/[\n\r]+/);
				for each(var sliceAuthorityLine:String in sliceAuthorityLines) {
					if(sliceAuthorityLine.length == 0)
						continue;
					var sliceAuthorityLineParts:Array = sliceAuthorityLine.split(" ");
					var sliceAuthority:SliceAuthority = new SliceAuthority(sliceAuthorityLineParts[0], sliceAuthorityLineParts[1], true);
					if(sliceAuthority.Name == "emulab.net")
						Main.geniHandler.GeniAuthorities.addItemAt(sliceAuthority, 0);
					else {
						var i:int;
						for(i = 0; i < Main.geniHandler.GeniAuthorities.length; i++) {
							var existingManager:SliceAuthority = Main.geniHandler.GeniAuthorities.getItemAt(i) as SliceAuthority;
							if(sliceAuthority.Name < existingManager.Name && existingManager.Name != "emulab.net")
								break;
						}
						Main.geniHandler.GeniAuthorities.addItemAt(sliceAuthority, i);
					}
				}
				
				Main.geniDispatcher.dispatchGeniAuthoritiesChanged();
			}
			else
			{
				//Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return newCalls.head;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			var crap:* = 2;
		}
	}
}
