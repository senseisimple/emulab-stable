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
	
	import protogeni.GeniEvent;
	import protogeni.StringUtil;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.Site;
	import protogeni.resources.Slice;
	
	/**
	 * TODO: Get working
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestResolvePlSlice extends Request
	{
		private var planetLabManager:PlanetlabAggregateManager;
		private var slice:Slice;
		
		public function RequestResolvePlSlice(newPlm:PlanetlabAggregateManager, newSlice:Slice):void
		{
			super("ResolveSlice (" + StringUtil.shortenString(newPlm.registryUrl, 15) + ")",
				"Resolving resources for " + newPlm.registryUrl,
				CommunicationUtil.resolvePl,
				true, true);
			this.ignoreReturnCode = true;
			op.timeout = 600;
			planetLabManager = newPlm;
			slice = newSlice;
			op.pushField([Main.geniHandler.CurrentUser.urn.full]);
			op.pushField([Main.geniHandler.CurrentUser.Credential]);	// credentials
			op.setExactUrl(planetLabManager.registryUrl);
			planetLabManager.lastRequest = this;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			return null;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			return null;
		}
	}
}
