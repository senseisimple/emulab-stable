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
	import protogeni.resources.Sliver;
	
	/**
	 * Gets the list of component managers from the clearinghouse using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestListComponents extends Request
	{
		private var startDiscoverResources:Boolean;
		private var startSlices:Boolean;
		
		public function RequestListComponents(shouldDiscoverResources:Boolean = true,
											  shouldStartSlices:Boolean = false):void
		{
			super("ListComponents",
				"Getting the information for the component managers",
				CommunicationUtil.listComponents);
			startDiscoverResources = shouldDiscoverResources;
			startSlices = shouldStartSlices;
		}
		
		/**
		 * Called immediately before the operation is run to add variables it may not have had when added to the queue
		 * @return Operation to be run
		 * 
		 */
		override public function start():Operation
		{
			op.addField("credential", Main.geniHandler.CurrentUser.Credential);
			return op;
		}
		
		// Should return Request or RequestQueueNode
		override public function complete(code:Number, response:Object):*
		{
			var newCalls:RequestQueue = new RequestQueue();
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.geniHandler.clearComponents();
				FlackCache.clearManagersOffline();
				
				for each(var obj:Object in response.value)
				{
					try {
						var newGm:GeniManager = null;
						var url:String = obj.url;
						var newUrn:IdnUrn = new IdnUrn(obj.urn);
						// Skip these managers for now...
						/*if(obj.hrn == "cron.loni.org.cm"
							|| obj.hrn == "cron.cct.lsu.edu.cm"
							|| obj.hrn == "wigims.cm"
							|| obj.hrn == "mygeni.cm"
							|| obj.hrn.toLowerCase().indexOf("etri") > 0)
							continue;*/
						// ProtoGENI Component Manager
						if(newUrn.name == "cm") {
							var newCm:ProtogeniComponentManager = new ProtogeniComponentManager();
							newCm.Hrn = obj.hrn;
							newCm.Url = obj.url.substr(0, obj.url.length-3);
							newCm.Urn = newUrn;
							newGm = newCm;
							if(newCm.Hrn == "ukgeni.cm" || newCm.Hrn == "utahemulab.cm")
								newCm.supportsIon = true;
							if(newCm.Hrn == "wail.cm" || newCm.Hrn == "utahemulab.cm")
								newCm.supportsGpeni = true;
							
						} else if(!Main.protogeniOnly)
						{
							var planetLabAm:PlanetlabAggregateManager = new PlanetlabAggregateManager();
							planetLabAm.Hrn = obj.hrn;
							planetLabAm.Url = StringUtil.makeSureEndsWith(obj.url, "/"); // needs this for forge...
							planetLabAm.registryUrl = planetLabAm.Url.replace("12346", "12345");
							planetLabAm.Urn = newUrn;
							newGm = planetLabAm;
						}
						
						var i:int;
						for(i = 0; i < Main.geniHandler.GeniManagers.length; i++) {
							var existingManager:GeniManager = Main.geniHandler.GeniManagers.getItemAt(i) as GeniManager;
							if(newGm.Urn.authority < existingManager.Urn.authority && existingManager.Urn.authority != "emulab.net")
								break;
						}
						Main.geniHandler.GeniManagers.addAt(newGm, i);
						
						if(startDiscoverResources)
						{
							newGm.Status = GeniManager.STATUS_INPROGRESS;
							if(newGm is AggregateManager)
								newCalls.push(new RequestGetVersionAm(newGm as AggregateManager));
							else if(newGm is ProtogeniComponentManager)
								newCalls.push(new RequestGetVersion(newGm as ProtogeniComponentManager));
						}
						Main.geniDispatcher.dispatchGeniManagerChanged(newGm);
					} catch(e:Error) {
						LogHandler.appendMessage(new LogMessage("", "ListComponents", "Couldn't add manager from list:\n" + obj.toString(), true, LogMessage.TYPE_END));
					}
				}
				
				if(startSlices) {
					if(Main.geniHandler.CurrentUser.userCredential.length > 0)
						newCalls.push(new RequestUserResolve());
					else if(Main.geniHandler.CurrentUser.sliceCredential.length > 0) {
						for each(var slice:Slice in Main.geniHandler.CurrentUser.slices) {
							if(slice.credential == Main.geniHandler.CurrentUser.sliceCredential) {
								Main.geniHandler.requestHandler.discoverSliceAllocatedResources(slice);
								break;
							}
						}
					}
				}
			}
			else
			{
				Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return newCalls.head;
		}
	}
}
