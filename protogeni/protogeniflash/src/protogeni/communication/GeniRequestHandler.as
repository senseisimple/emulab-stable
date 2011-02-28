/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
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
	
	import protogeni.Util;
	import protogeni.display.DisplayUtil;
	import protogeni.display.InitialUserWindow;
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	import protogeni.resources.SliverCollection;
    
    // Handles all the XML-RPC calls
	public class GeniRequestHandler
	{
		public function GeniRequestHandler()
		{
		}
		
		public var queue:RequestQueue = new RequestQueue(true);
		public var forceStop:Boolean = false;
		public var isPaused:Boolean = false;
		
		public static var MAX_RUNNING:int = 3;
		
		// Run everything from the very beginning
		public function startInitiationSequence(tryAuthenticate:Boolean = false):void
		{
			if(Main.geniHandler.unauthenticatedMode && !tryAuthenticate) {
				pushRequest(new RequestListComponentsPublic());
				Main.Application().showAuthenticate();
				
			} else if(Main.geniHandler.unauthenticatedMode || Main.geniHandler.forceAuthority == null) {
				DisplayUtil.viewInitialUserWindow();
			} else {
				startAuthenticatedInitiationSequence();
			}
		}
		
		public function startAuthenticatedInitiationSequence():void
		{
			if(Main.geniHandler.unauthenticatedMode) {
				DisplayUtil.viewInitialUserWindow();
				return;
			}
			pushRequest(new RequestGetCredential());
			pushRequest(new RequestGetKeys());
			loadListAndComponentManagersAndSlices();
			Main.Application().hideAuthenticate();
		}
		
		public function loadListAndComponentManagers():void
		{
			if(Main.geniHandler.unauthenticatedMode)
				pushRequest(new RequestListComponentsPublic());
			else
				pushRequest(new RequestListComponents(true, false));
			
		}
		
		public function loadListAndComponentManagersAndSlices():void
		{
			if(Main.geniHandler.unauthenticatedMode)
				pushRequest(new RequestListComponentsPublic());
			else
				pushRequest(new RequestListComponents(true, true));
		}
		
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
		
		public function createSlice(name:String) : void
		{
			var newSlice:Slice = new Slice();
			newSlice.hrn = name;
			newSlice.urn = Util.makeUrn(Main.geniHandler.CurrentUser.authority.Authority, "slice", name);
			newSlice.creator = Main.geniHandler.CurrentUser;
			pushRequest(new RequestSliceResolve(newSlice, true));
		}
		
		public function submitSlice(slice:Slice):void
		{
			var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(slice.urn);
			if(old != null && old.hasAllocatedResources())
			{
				var newSlivers:SliverCollection = new SliverCollection();
				var deleteSlivers:SliverCollection = new SliverCollection();
				var updateSlivers:SliverCollection = slice.slivers.clone();
				for each(var s:Sliver in old.slivers)
				{
					if(slice.slivers.getByGm(s.manager) == null)
						deleteSlivers.addItem(s);
				}
				for each(s in slice.slivers)
				{
					if(old.slivers.getByGm(s.manager) == null)
					{
						newSlivers.addItem(s);
						updateSlivers.removeItemAt(updateSlivers.getItemIndex(s));
					}
				}
				Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
				
				// Create
				for each(var sliver:Sliver in newSlivers) {
					if(sliver.manager is AggregateManager)
						pushRequest(new RequestSliverCreateAm(sliver));
					else if(sliver.manager is ProtogeniComponentManager)
						pushRequest(new RequestSliverCreate(sliver));
				}
					
				// Update
				for each(sliver in updateSlivers) {
					pushRequest(new RequestSliverUpdate(sliver));
				}
				
				// Delete
				for each(sliver in deleteSlivers) {
					if(sliver.manager is AggregateManager)
						pushRequest(new RequestSliverDeleteAm(sliver));
					else if(sliver.manager is ProtogeniComponentManager)
						pushRequest(new RequestSliverDelete(sliver));
				}
			} else {
				// Create
				Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
				for each(sliver in slice.slivers) {
					if(sliver.manager is AggregateManager)
						pushRequest(new RequestSliverCreateAm(sliver));
					else if(sliver.manager is ProtogeniComponentManager)
						pushRequest(new RequestSliverCreate(sliver));
				}
			}
		}
		
		public function refreshSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers) {
				if(sliver.manager is AggregateManager)
					pushRequest(new RequestSliverStatusAm(sliver));
				else if(sliver.manager is ProtogeniComponentManager)
					pushRequest(new RequestSliverStatus(sliver));
			}
		}
		
		public function deleteSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				if(sliver.manager is AggregateManager)
					pushRequest(new RequestSliverDeleteAm(sliver));
				else if(sliver.manager is ProtogeniComponentManager)
					pushRequest(new RequestSliverDelete(sliver));
			}
		}
		
		public function startSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				pushRequest(new RequestSliverStart(sliver));
			}
		}
		
		public function stopSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				pushRequest(new RequestSliverStop(sliver));
			}
		}
		
		public function restartSlice(slice:Slice):void
		{
			Main.geniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				pushRequest(new RequestSliverRestart(sliver));
			}
		}
		
		public function pushRequest(newRequest : Request, forceStart:Boolean = true) : void
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
		
		public function start() : void
		{
			isPaused = false;
			forceStop = false;
			if(!queue.readyToStart())
				return;
			
			if(queue.workingCount() < MAX_RUNNING) {
				var start:Request = queue.nextAndProgress();
				start.running = true;
				var op:Operation = start.start();
				op.call(complete, failure);
				Main.Application().setStatus(start.name, false);
				LogHandler.appendMessage(new LogMessage(op.getUrl(), start.name, op.getSent(), false, LogMessage.TYPE_START));
				
				Main.geniDispatcher.dispatchQueueChanged();

				this.start()
			}
		}
		
		public function pause():void
		{
			isPaused = true;
		}
		
		public function stop():void
		{
			pause();
			removeAll();
		}
		
		public function removeAll():void {
			while(queue.head != null) {
				remove(queue.head.item as Request, true);
			}
		}
		
		public function remove(r:Request, showAction:Boolean = true):void
		{
			if(r.running)
			{
				Main.Application().setStatus(r.name + " canceled!", false);
				r.cancel();
			}
			queue.remove(queue.getRequestQueueNodeFor(r));
			if(showAction)
			{
				var url:String = r.op.getUrl();
				var name:String = r.name;
				LogHandler.appendMessage(new LogMessage(url, name + "Removed", "Request removed", false, LogMessage.TYPE_OTHER));
				if(!queue.working())
					tryNext();
			}
			Main.geniDispatcher.dispatchQueueChanged();
		}
		
		private function failure(node:Request, event : ErrorEvent, fault : MethodFault) : void
		{
			node.running = false;
			remove(node, false);

			// Get and give general info for the failure
			var failMessage:String = "";
			var msg : String = "";
			if (fault != null)
			{
				msg = fault.getFaultString();
				failMessage += "\nFAILURE fault: " + node.name + ": " + msg;
			}
			else
			{
				msg = event.toString();
				failMessage += "\nFAILURE event: " + node.name + ": " + msg;
				if(msg.search("#2048") > -1)
					failMessage += "\nStream error, possibly due to server error";
				else if(msg.search("#2032") > -1) {
					if(Main.geniHandler.unauthenticatedMode)
						failMessage += "\nIO Error, possibly due to server problems";
					else
						failMessage += "\nIO Error, possibly due to server problems or you have no SSL certificate";
				}
					
			}
			failMessage += "\nURL: " + node.op.getUrl();
			LogHandler.appendMessage(new LogMessage(node.op.getUrl(), node.name, failMessage, true, LogMessage.TYPE_END));
			Main.Application().setStatus(node.name + " failed!", true);
			if(!node.continueOnError)
			{
				LogHandler.viewConsole();
			} else {
				// Find out what to do next
				var next : * = node.fail(event, fault);
				node.cleanup();
				if (next != null)
					queue.push(next);
				
				tryNext();
			}
			
			if(msg.search("#2048") > -1 || msg.search("#2032") > -1)
			{
				if(!Main.geniHandler.unauthenticatedMode
					&& (Main.geniHandler.CurrentUser.credential == null || Main.geniHandler.CurrentUser.credential.length == 0))
				{
					Alert.show("It appears that you may have never run this program before.  In order to run correctly, you will need to follow the steps at https://www.protogeni.net/trac/protogeni/wiki/FlashClientSetup.  Would you like to visit now?", "Set up", Alert.YES | Alert.NO, Main.Application(),
						function runSetup(e:CloseEvent):void
						{
							if(e.detail == Alert.YES)
							{
								Util.showSetup();
							}
						});
				}
			}
		}
		
		private function complete(node:Request, code : Number, response : Object) : void
		{
			if(node.removeImmediately)
			{
				node.running = false;
				remove(node, false);
			}
			
			var next:*;
			try
			{
				// Output completed
				if(code != CommunicationUtil.GENIRESPONSE_SUCCESS && !node.ignoreReturnCode)
				{
					Main.Application().setStatus(node.name + " done", true);
					LogHandler.appendMessage(new LogMessage(node.op.getUrl(), CommunicationUtil.GeniresponseToString(code), node.op.getResponse(), true, LogMessage.TYPE_END));
				} else {
					Main.Application().setStatus(node.name + " done", false);
					LogHandler.appendMessage(new LogMessage(node.op.getUrl(), node.name, node.op.getResponse(), false, LogMessage.TYPE_END));
				}
				next = node.complete(code, response);
			}
			catch (e : Error)
			{
				codeFailure(node.name, "Error caught in RPC-Handler Complete", e, !queue.front().continueOnError);
				node.removeImmediately = true;
				if(node.running)
				{
					node.running = false;
					remove(node, false);
				}
				if(!node.continueOnError)
					return;
			}
			
			// Find out what to do next
			if(node.removeImmediately)
			{
				if (next != null)
					queue.push(next);
				node.cleanup();
			}
			
			tryNext();
		}
		
		public function tryNext():void
		{
			if(!forceStop && !isPaused)
				start();
			else
				forceStop = false;
		}
		
		public function clearAll():void
		{
			// Should probably be different
			this.queue.head = null;
		}
		
		public function codeFailure(name:String, detail:String = "", e:Error = null, stop:Boolean = true) : void
		{
			if(stop)
			{
				LogHandler.viewConsole();
				forceStop = true;
			}
				
			if(e != null)
				LogHandler.appendMessage(new LogMessage("", "Code Failure: " + name,detail + "\n\n" + e.toString() + "\n\n" + e.getStackTrace(),true,LogMessage.TYPE_END));
			else
				LogHandler.appendMessage(new LogMessage("", "Code Failure: " + name,detail,true,LogMessage.TYPE_END));
			
		}
	}
}