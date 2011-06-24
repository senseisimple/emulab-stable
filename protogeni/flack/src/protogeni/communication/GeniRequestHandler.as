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
	
	import mx.controls.Alert;
	import mx.events.CloseEvent;
	
	import protogeni.NetUtil;
	import protogeni.display.DisplayUtil;
	import protogeni.resources.GeniManager;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	import protogeni.resources.SliverCollection;

	/**
	 * Handles all XML-RPC and HTTP requests
	 * 
	 * @author mstrum
	 * 
	 */
	public final class GeniRequestHandler
	{
		public var queue:RequestQueue = new RequestQueue(true);
		public var forceStop:Boolean = false;
		public var isPaused:Boolean = false;
		
		[Bindable]
		public var maxRunning:int = 3;
		
		public function GeniRequestHandler()
		{
		}

		/**
		 * Starts the sequence to get all GENI information
		 * 
		 * @param tryAuthenticate Does the user want to authenticate?
		 * 
		 */
		public function startInitiationSequence(tryAuthenticate:Boolean = false):void
		{
			if(Main.geniHandler.unauthenticatedMode && !tryAuthenticate) {
				pushRequest(new RequestListComponentsPublic());
				Main.Application().showAuthenticate();
			} else if(Main.geniHandler.unauthenticatedMode
				|| Main.geniHandler.CurrentUser.authority == null) {
				DisplayUtil.viewInitialUserWindow();
			} else {
				startAuthenticatedInitiationSequence();
			}
		}
		
		/**
		 * Starts authenticated GENI calls
		 * 
		 */
		public function startAuthenticatedInitiationSequence(getSlices:Boolean = true):void
		{
			if(Main.geniHandler.unauthenticatedMode) {
				DisplayUtil.viewInitialUserWindow();
				return;
			}
			if(Main.geniHandler.CurrentUser.Credential.length == 0) {
				pushRequest(new RequestGetCredential());
				pushRequest(new RequestGetKeys());
			}
			loadListAndComponentManagers(getSlices);
			Main.Application().hideAuthenticate();
		}
		
		/**
		 * Adds calls to get the list of component managers
		 * 
		 */
		public function loadListAndComponentManagers(getSlices:Boolean = false):void
		{
			/*var newCm:ProtogeniComponentManager = new ProtogeniComponentManager();
			newCm.isAm = true;
			newCm.Hrn = "beelab.cm";
			newCm.Url = "https://myboss.geelab.geni.emulab.net/protogeni/xmlrpc/am";
			newCm.Urn = new IdnUrn("urn:publicid:IDN+geelab.geni.emulab.net+authority+cm");
			Main.geniHandler.GeniManagers.add(newCm);
			newCm.Status = GeniManager.STATUS_INPROGRESS;
			this.pushRequest(new RequestGetVersionAm(newCm));*/
			
			if(Main.geniHandler.unauthenticatedMode)
				pushRequest(new RequestListComponentsPublic());
			else
				pushRequest(new RequestListComponents(true, getSlices));
			
		}
		
		/**
		 * Adds calls to get advertisements from all of the managers
		 * 
		 */
		public function loadComponentManagers():void
		{
			for each(var cm:ProtogeniComponentManager in Main.geniHandler.GeniManagers)
			{
				if(Main.geniHandler.unauthenticatedMode) {
					cm.Status = GeniManager.STATUS_INPROGRESS;
					Main.geniDispatcher.dispatchGeniManagerChanged(cm);
					pushRequest(new RequestDiscoverResourcesPublic(cm));
				}
				else
					pushRequest(new RequestGetVersion(cm));
			}
		}
		
		public function discoverSliceAllocatedResources(slice:Slice):void {
			for each(var manager:GeniManager in Main.geniHandler.GeniManagers) {
				var newSliver:Sliver = new Sliver(slice, manager);
				if(manager.isAm)
					this.pushRequest(new RequestSliverListResourcesAm(newSliver));
				else
					this.pushRequest(new RequestSliverGet(newSliver));
			}
		}
		
		/**
		 * Creates a blank slice for the user to use
		 * 
		 * @param name
		 * 
		 */
		public function createSlice(name:String):void
		{
			var newSlice:Slice = new Slice();
			newSlice.hrn = name;
			newSlice.urn = IdnUrn.makeFrom(Main.geniHandler.CurrentUser.authority.Urn.authority, "slice", name);
			newSlice.creator = Main.geniHandler.CurrentUser;
			pushRequest(new RequestSliceResolve(newSlice, true));
		}
		
		/**
		 * Takes a slice that has been prepared and tries to allocate all of the resources
		 * 
		 * @param slice Slice with everything the user wants to allocate
		 * 
		 */
		public function submitSlice(slice:Slice):void
		{
			this.isPaused = false;
			Main.geniDispatcher.dispatchSliceChanged(slice);
			var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(slice.urn.full);
			if(old != null && old.hasAllocatedResources())
			{
				var newSlivers:SliverCollection = new SliverCollection();
				var deleteSlivers:SliverCollection = new SliverCollection();
				var updateSlivers:SliverCollection = slice.slivers.clone();
				for each(var s:Sliver in old.slivers.collection)
				{
					if(slice.slivers.getByGm(s.manager) == null)
						deleteSlivers.add(s);
				}
				for each(s in slice.slivers.collection)
				{
					if(old.slivers.getByGm(s.manager) == null)
					{
						newSlivers.add(s);
						updateSlivers.remove(s);
					}
				}
				Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
				
				// Create
				var addDelay:Boolean = false;
				for each(var sliver:Sliver in newSlivers.collection) {
					var request:Request;
					if(sliver.manager.isAm)
						request = new RequestSliverCreateAm(sliver);
					else if(sliver.manager is ProtogeniComponentManager)
						request = new RequestSliverCreate(sliver);
					
					// Add a delay for the other slivers in case there is any stitching
					if(addDelay)
						request.op.delaySeconds = 15;
					else
						addDelay = true;
					
					pushRequest(request);
				}
				
				// Update
				for each(sliver in updateSlivers.collection) {
					pushRequest(new RequestSliverUpdate(sliver));
				}
				
				// Delete
				for each(sliver in deleteSlivers.collection) {
					if(sliver.manager.isAm)
						pushRequest(new RequestSliverDeleteAm(sliver));
					else if(sliver.manager is ProtogeniComponentManager)
						pushRequest(new RequestSliverDelete(sliver));
				}
			} else {
				// Create
				Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
				for each(sliver in slice.slivers.collection) {
					sliver.created = false;
					sliver.staged = false;
				}
				for each(sliver in slice.slivers.collection) {
					if(sliver.manager.isAm)
						pushRequest(new RequestSliverCreateAm(sliver));
					else if(sliver.manager is ProtogeniComponentManager)
						pushRequest(new RequestSliverCreate(sliver));
				}
			}
		}
		
		/**
		 * Gets the status for all of the slivers in the slice
		 * 
		 * @param slice Slice to get the status from
		 * @param skipDone Skip slivers which are ready
		 * 
		 */
		public function refreshSlice(slice:Slice, skipDone:Boolean = false):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers.collection) {
				if(skipDone && sliver.status == Sliver.STATUS_READY)
					continue;
				if(sliver.manager.isAm)
					pushRequest(new RequestSliverStatusAm(sliver));
				else if(sliver.manager is ProtogeniComponentManager)
					pushRequest(new RequestSliverStatus(sliver));
			}
		}
		
		/**
		 * Deallocate all resources in the slice
		 * 
		 * @param slice Slice to release resources from
		 * 
		 */
		public function deleteSlice(slice:Slice):void
		{
			this.isPaused = false;
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			// SliceRemove doesn't work because the slice hasn't expired yet...
			for each(var sliver:Sliver in slice.slivers.collection)
			{
				if(sliver.manager.isAm)
					this.pushRequest(new RequestSliverDeleteAm(sliver));
				else if(sliver.manager is ProtogeniComponentManager)
					this.pushRequest(new RequestSliverDelete(sliver));
			}
			Main.geniDispatcher.dispatchSliceChanged(slice);
		}
		
		/**
		 * Bind unbound nodes in a slice
		 * 
		 * @param slice Slice which to bind all nodes
		 * 
		 */
		public function embedSlice(slice:Slice):void {
			for each(var sliver:Sliver in slice.slivers.collection)
			{
				pushRequest(new RequestSliceEmbedding(sliver));
			}
		}
		
		/**
		 * Starts all of the resources in a slice
		 * 
		 * @param slice Slice which to start resources
		 * 
		 */
		public function startSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers.collection)
			{
				pushRequest(new RequestSliverStart(sliver));
			}
		}
		
		/**
		 * Stops a slice
		 * 
		 * @param slice Slice to stop
		 * 
		 */
		public function stopSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers.collection)
			{
				pushRequest(new RequestSliverStop(sliver));
			}
		}
		
		/**
		 * Restarts a slice
		 * 
		 * @param slice Slice to restart
		 * 
		 */
		public function restartSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers.collection)
			{
				pushRequest(new RequestSliverRestart(sliver));
			}
		}
		
		/**
		 * Adds a request to be called to the request queue
		 * 
		 * @param newRequest Request to add to the queue
		 * @param forceStart Start the queue immediately?
		 * 
		 */
		public function pushRequest(newRequest:Request, forceStart:Boolean = true):void
		{
			if (newRequest != null)
			{
				queue.push(newRequest);
				if (queue.readyToStart() && forceStart && !isPaused)
				{
					start();
				}
			}
		}
		
		/**
		 * Start the request queue
		 * 
		 * @param continuous Continue making calls?
		 * 
		 */
		public function start(continuous:Boolean = true):void
		{
			isPaused = false;
			forceStop = false;
			if(!queue.readyToStart())
				return;
			
			if(queue.workingCount() < maxRunning) {
				var start:Request = queue.nextAndProgress();
				start.running = true;
				var op:Operation = start.start();
				if(op == null) {
					start.running = false;
					this.remove(start);
				} else {
					start.numTries++;
					op.call(complete, failure);
					Main.Application().setStatus(start.name, false);
					LogHandler.appendMessage(new LogMessage(op.getUrl(),
						start.name,
						op.getSent(),
						false,
						LogMessage.TYPE_START));
					
					Main.geniDispatcher.dispatchQueueChanged();
				}
				
				if(continuous)
					this.start();
				else
					pause();
			}
		}
		
		/**
		 * Stop the queue from making any new requests until the user flags to start again
		 * 
		 */
		public function pause():void
		{
			isPaused = true;
		}
		
		/**
		 * Pauses and removes all requests in the queue
		 * 
		 */
		public function stop():void
		{
			pause();
			removeAll();
		}
		
		/**
		 * Removes all requests in the queue
		 * 
		 */
		public function removeAll():void {
			while(queue.head != null) {
				remove(queue.head.item as Request, true);
			}
		}
		
		/**
		 * Removes a request from the queue
		 * @param request Request to add
		 * @param showAction Show the action in the logs?
		 * 
		 */
		public function remove(request:Request, showAction:Boolean = true):void
		{
			if(request.running)
			{
				Main.Application().setStatus(request.name + " canceled!", false);
				request.cancel();
			}
			queue.remove(queue.getRequestQueueNodeFor(request));
			if(showAction)
			{
				var url:String = request.op.getUrl();
				var name:String = request.name;
				LogHandler.appendMessage(new LogMessage(url,
														name + "Removed",
														"Request removed",
														false,
														LogMessage.TYPE_OTHER));
				if(!queue.working())
					tryNext();
			}
			Main.geniDispatcher.dispatchQueueChanged();
		}
		
		/**
		 * Called after request failures
		 * 
		 * @param node Request which failed
		 * @param event Related event
		 * @param fault Related fault
		 * 
		 */
		private function failure(request:Request, event:ErrorEvent, fault:MethodFault):void
		{
			request.running = false;
			remove(request, false);
			
			// Get and give general info for the failure
			var failMessage:String = "";
			var msg:String = "";
			if (fault != null)
			{
				msg = fault.getFaultString();
				failMessage += "\nFAILURE fault: " + request.name + ": " + msg;
			}
			else
			{
				msg = event.toString();
				failMessage += "\nFAILURE event: " + request.name + ": " + msg;
				if(msg.search("#2048") > -1)
					failMessage += "\nStream error, possibly due to server error\n\n****Very possible that this server has not added a Flash socket security policy server****";
				else if(msg.search("#2032") > -1) {
					if(Main.geniHandler.unauthenticatedMode)
						failMessage += "\nIO Error, possibly due to server problems";
					else
						failMessage += "\nIO Error, possibly due to server problems or you have no SSL certificate";
				}
				
			}
			failMessage += "\nURL: " + request.op.getUrl();
			LogHandler.appendMessage(new LogMessage(request.op.getUrl(),
													request.name,
													failMessage,
													true,
													LogMessage.TYPE_END));
			Main.Application().setStatus(request.name + " failed!", true);
			if(!request.continueOnError)
			{
				LogHandler.viewConsole();
			} else {
				// Find out what to do next
				var next:* = request.fail(event, fault);
				request.cleanup();
				if (next != null)
					queue.push(next);
				
				tryNext();
			}
			
			/*if(msg.search("#2048") > -1 || msg.search("#2032") > -1)
			{
				if(!Main.geniHandler.unauthenticatedMode
					&& Main.geniHandler.CurrentUser.Credential.length == 0)
				{
					Alert.show("It appears that you may have never run this program before.  In order to run correctly, you will need to follow the steps at https://www.protogeni.net/trac/protogeni/wiki/FlashClientSetup.  Would you like to visit now?", "Set up", Alert.YES | Alert.NO, Main.Application(),
						function runSetup(e:CloseEvent):void
						{
							if(e.detail == Alert.YES)
							{
								NetUtil.showBecomingAUser();
							}
						});
				}
			}*/
		}
		
		/**
		 * Called after successful request
		 * 
		 * @param request
		 * @param code ProtoGENI response code
		 * @param response
		 * 
		 */
		private function complete(request:Request, code:Number, response:Object):void
		{
			if(request.removeImmediately)
			{
				request.running = false;
				remove(request, false);
			}
			
			var next:*;
			try
			{
				if(code == CommunicationUtil.GENIRESPONSE_BUSY) {
					Main.Application().setStatus(request.name + " busy", true);
					if(request.numTries == 8) {
						LogHandler.appendMessage(new LogMessage(request.op.getUrl(),
																request.name + " busy",
																"Reach limit of retries",
																true,
																LogMessage.TYPE_END ));
					} else {
						LogHandler.appendMessage(new LogMessage(request.op.getUrl(),
																request.name + " busy",
																"Preparing to retry",
																true,
																LogMessage.TYPE_END ));
						request.op.delaySeconds = 10;
						request.forceNext = true;
						next = request;
					}
				} else {
					if(code != CommunicationUtil.GENIRESPONSE_SUCCESS && !request.ignoreReturnCode)
					{
						Main.Application().setStatus(request.name + " done", true);
						LogHandler.appendMessage(new LogMessage(request.op.getUrl(),
																CommunicationUtil.GeniresponseToString(code),
							"------------------------\nResponse:\n" +
							request.op.getResponse() +
							"\n\n------------------------\nRequest:\n" + request.op.getSent(),
							true, LogMessage.TYPE_END));
					} else {
						Main.Application().setStatus(request.name + " done", false);
						LogHandler.appendMessage(new LogMessage(request.op.getUrl(),
																request.name,
																request.op.getResponse(),
																false,
																LogMessage.TYPE_END));
					}
					next = request.complete(code, response);
				}
			}
			catch (e:Error)
			{
				codeFailure(request.name, "Error caught in RPC-Handler Complete",
							e,
							!queue.front().continueOnError);
				request.removeImmediately = true;
				if(request.running)
				{
					request.running = false;
					remove(request, false);
				}
				if(!request.continueOnError)
					return;
			}
			
			// Find out what to do next
			if(request.removeImmediately)
			{
				if (next != null)
					queue.push(next);
				if(next != request)
					request.cleanup();
			}
			
			tryNext();
		}
		
		/**
		 * Call one request
		 * 
		 */
		public function step():void {
			isPaused = false;
			forceStop = false;
			tryNext(false);
		}
		
		/**
		 * Trys to start the next request
		 * 
		 * @param continuous Continue making calls?
		 * 
		 */
		public function tryNext(continuous:Boolean = true):void
		{
			if(!forceStop && !isPaused)
				start(continuous);
			else
				forceStop = false;
		}
		
		/**
		 * Just sets the head of the queue to null instead of removing all requests
		 * 
		 */
		public function clearAll():void
		{
			// Should probably be different
			this.queue.head = null;
		}
		
		/**
		 * Something wrong happend in code instead of the request
		 * 
		 * @param name
		 * @param detail
		 * @param e
		 * @param stop
		 * 
		 */
		public function codeFailure(name:String,
									detail:String = "",
									e:Error = null,
									stop:Boolean = true):void
		{
			if(stop)
			{
				LogHandler.viewConsole();
				forceStop = true;
			}
			
			if(e != null)
				LogHandler.appendMessage(new LogMessage("",
														"Code Failure: " + name,detail + "\n\n" + e.toString() + "\n\n" + e.getStackTrace(),
														true,
														LogMessage.TYPE_END));
			else
				LogHandler.appendMessage(new LogMessage("",
														"Code Failure: " + name,
														detail,
														true,
														LogMessage.TYPE_END));
			
		}
	}
}