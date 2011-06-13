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
	
	import protogeni.StringUtil;
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	
	/**
	 * Gets the GENI AM API version used by the manager using the GENI AM API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestGetVersionAm extends Request
	{
		private var aggregateManager:GeniManager;
		
		public function RequestGetVersionAm(newManager:GeniManager):void
		{
			super("GetVersion (" + StringUtil.shortenString(newManager.Url, 15) + ")",
				"Getting the version of the aggregate manager for " + newManager.Hrn,
				CommunicationUtil.getVersionAm,
				true,
				true,
				true);
			ignoreReturnCode = true;
			aggregateManager = newManager;
			
			op.setExactUrl(aggregateManager.Url);
		}
		
		// Should return Request or RequestQueueNode
		override public function complete(code:Number, response:Object):*
		{
			var r:Request = null;
			try
			{
				aggregateManager.Version = response.geni_api;
				r = new RequestListResourcesAm(aggregateManager);
				r.forceNext = true;
			}
			catch(e:Error)
			{
			}
			
			return r;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			aggregateManager.errorMessage = event.toString();
			aggregateManager.errorDescription = "";
			if(aggregateManager.errorMessage.search("#2048") > -1)
				aggregateManager.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + aggregateManager.VisitUrl();
			else if(aggregateManager.errorMessage.search("#2032") > -1)
				aggregateManager.errorDescription = "IO error, possibly due to the server being down";
			else if(aggregateManager.errorMessage.search("timed"))
				aggregateManager.errorDescription = event.text;
			
			aggregateManager.Status = GeniManager.STATUS_FAILED;
			Main.geniDispatcher.dispatchGeniManagerChanged(aggregateManager);
			
			return null;
		}
		
		override public function cancel():void
		{
			aggregateManager.Status = GeniManager.STATUS_UNKOWN;
			Main.geniDispatcher.dispatchGeniManagerChanged(aggregateManager);
			op.cleanup();
		}
	}
}
