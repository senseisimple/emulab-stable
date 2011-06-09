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
	import flash.utils.ByteArray;
	
	import mx.utils.Base64Decoder;
	
	import protogeni.StringUtil;
	import protogeni.resources.GeniManager;
	import protogeni.resources.ProtogeniComponentManager;
	
	/**
	 * Gets the manager's advertisement using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestDiscoverResources extends Request
	{
		private var componentManager:ProtogeniComponentManager;
		
		public function RequestDiscoverResources(newManager:ProtogeniComponentManager):void
		{
			super("DiscoverResources (" + StringUtil.shortenString(newManager.Hrn, 15) + ")",
				"Discovering resources for " + newManager.Hrn,
				CommunicationUtil.discoverResources,
				true,
				true,
				false);
			op.timeout = 60;
			componentManager = newManager;
			
			// Build up the args
			op.addField("credentials", [Main.geniHandler.CurrentUser.Credential]);
			op.addField("compress", true);
			op.addField("rspec_version", componentManager.outputRspecVersion);
			op.setUrl(newManager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				var decodor:Base64Decoder = new Base64Decoder();
				decodor.decode(response.value);
				var bytes:ByteArray = decodor.toByteArray();
				bytes.uncompress();
				var decodedRspec:String = bytes.toString();
				
				componentManager.Rspec = new XML(decodedRspec);
				componentManager.rspecProcessor.processResourceRspec(after);
			}
			else
			{
				componentManager.errorMessage = response.output;
				componentManager.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + componentManager.errorMessage;
				componentManager.Status = GeniManager.STATUS_FAILED;
				this.removeImmediately = true;
				Main.geniDispatcher.dispatchGeniManagerChanged(componentManager);
			}
			
			return null;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			componentManager.errorMessage = event.toString();
			componentManager.errorDescription = "";
			if(componentManager.errorMessage.search("#2048") > -1)
				componentManager.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + componentManager.VisitUrl();
			else if(componentManager.errorMessage.search("#2032") > -1)
				componentManager.errorDescription = "IO error, possibly due to the server being down";
			else if(componentManager.errorMessage.search("timed"))
				componentManager.errorDescription = event.text;
			
			componentManager.Status = GeniManager.STATUS_FAILED;
			Main.geniDispatcher.dispatchGeniManagerChanged(componentManager);
			
			return null;
		}
		
		override public function cancel():void
		{
			componentManager.Status = GeniManager.STATUS_UNKOWN;
			Main.geniDispatcher.dispatchGeniManagerChanged(componentManager);
			op.cleanup();
		}
		
		private function after(junk:GeniManager):void {
			cleanup();
		}
		
		override public function cleanup():void
		{
			if(componentManager.Status == GeniManager.STATUS_INPROGRESS)
				componentManager.Status = GeniManager.STATUS_FAILED;
			running = false;
			Main.geniHandler.requestHandler.remove(this, false);
			Main.geniDispatcher.dispatchGeniManagerChanged(componentManager);
			op.cleanup();
			Main.geniHandler.mapHandler.drawMap();
			Main.geniHandler.requestHandler.tryNext();
		}
	}
}
