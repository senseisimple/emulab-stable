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
	
	public final class RequestListResourcesAmPublic extends Request
	{
		private var aggregateManager:AggregateManager;
		
		public function RequestListResourcesAmPublic(newAm:AggregateManager):void
		{
			super("ListResourcesAmPublic (" + StringUtil.shortenString(newAm.Url, 15) + ")",
				"Listing resources for " + newAm.Url,
				null,
				true,
				true,
				false);
			aggregateManager = newAm;
			op.setExactUrl(aggregateManager.Url);
			op.type = Operation.HTTP;
			aggregateManager.lastRequest = this;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var r:Request = null;
			
			try
			{
				aggregateManager.Rspec = new XML(response);
				aggregateManager.rspecProcessor.processResourceRspec(cleanup);
				
			} catch(e:Error)
			{
				//am.errorMessage = response;
				//am.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + am.errorMessage;
				aggregateManager.Status = GeniManager.STATUS_FAILED;
				this.removeImmediately = true;
				Main.geniDispatcher.dispatchGeniManagerChanged(aggregateManager);
			}
			
			return r;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			aggregateManager.errorMessage = event.toString();
			aggregateManager.errorDescription = event.text;
			if(aggregateManager.errorMessage.search("#2048") > -1)
				aggregateManager.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + aggregateManager.VisitUrl();
			else if(aggregateManager.errorMessage.search("#2032") > -1)
				aggregateManager.errorDescription = "IO error, possibly due to the server being down";
			
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
		
		override public function cleanup():void
		{
			if(aggregateManager.Status == GeniManager.STATUS_INPROGRESS)
				aggregateManager.Status = GeniManager.STATUS_FAILED;
			running = false;
			Main.geniHandler.requestHandler.remove(this, false);
			Main.geniDispatcher.dispatchGeniManagerChanged(aggregateManager);
			op.cleanup();
			Main.geniHandler.requestHandler.tryNext();
		}
	}
}
