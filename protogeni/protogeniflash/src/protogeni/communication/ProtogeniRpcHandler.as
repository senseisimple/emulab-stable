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
	import flash.events.SecurityErrorEvent;
	import flash.utils.ByteArray;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	import mx.utils.Base64Decoder;
	
	import protogeni.Util;
	import protogeni.communication.Operation;
	import protogeni.display.DisplayUtil;
	import protogeni.resources.ComponentManager;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
    
    // Handles all the XML-RPC calls
	public class ProtogeniRpcHandler
	{
		public function ProtogeniRpcHandler()
		{
		}
		
		public var queue:RequestQueue = new RequestQueue(true);
		public var forceStop:Boolean = false;
		public var isPaused:Boolean = false;
		
		// Run everything from the very beginning
		public function startInitiationSequence():void
		{
			pushRequest(new RequestGetCredential());
			pushRequest(new RequestGetKeys());
			loadListAndComponentManagersAndSlices();
		}
		
		public function loadListAndComponentManagers():void
		{
			pushRequest(new RequestListComponents(true, false));
		}
		
		public function loadListAndComponentManagersAndSlices():void
		{
			pushRequest(new RequestListComponents(true, true));
		}
		
		public function loadComponentManagers():void
		{
			for each(var cm:ComponentManager in Main.protogeniHandler.ComponentManagers)
			{
				pushRequest(new RequestDiscoverResources(cm));
			}
		}
		
		public function createSlice(name:String) : void
		{
			var newSlice:Slice = new Slice();
			newSlice.hrn = name;
			newSlice.urn = Util.makeUrn(CommunicationUtil.defaultAuthority, "slice", name);
			newSlice.creator = Main.protogeniHandler.CurrentUser;
			pushRequest(new RequestSliceResolve(newSlice, true));
		}
		
		public function submitSlice(slice:Slice):void
		{
			var old:Slice = Main.protogeniHandler.CurrentUser.slices.getByUrn(slice.urn);
			if(old != null && old.hasAllocatedResources())
			{
				// Update
				Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
				for each(var sliver:Sliver in slice.slivers)
					pushRequest(new RequestSliverUpdate(sliver));
			} else {
				// Create
				Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
				for each(sliver in slice.slivers)
					pushRequest(new RequestSliverCreate(sliver));
			}
		}
		
		public function refreshSlice(slice:Slice):void
		{
			Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
				pushRequest(new RequestSliverStatus(sliver));
		}
		
		public function deleteSlice(slice:Slice):void
		{
			Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				pushRequest(new RequestSliverDelete(sliver));
			}
		}
		
		public function startSlice(slice:Slice):void
		{
			Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				pushRequest(new RequestSliverStart(sliver));
			}
		}
		
		public function stopSlice(slice:Slice):void
		{
			Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				pushRequest(new RequestSliverStop(sliver));
			}
		}
		
		public function restartSlice(slice:Slice):void
		{
			Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
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
				if (queue.readyToStart() && forceStart)
				{
					start();
				}
			}
		}
		
		public function start() : void
		{
			isPaused = false;
			if(!queue.readyToStart())
				return;
			
			var start:Request = queue.nextAndProgress();
			start.running = true;
			var op:Operation = start.start();
			op.call(complete, failure);
			Main.log.setStatus(start.name, false);
			Main.log.appendMessage(new LogMessage(op.getUrl(), "Send: " + start.name, op.getSendXml(), false, LogMessage.TYPE_START));
				
			Main.protogeniHandler.dispatchQueueChanged();
			
			this.start();
		}
		
		public function pause():void
		{
			isPaused = true;
		}
		
		public function remove(r:Request, showAction:Boolean = true):void
		{
			if(r.running)
			{
				Main.log.setStatus(r.name + " canceled!", false);
				r.cancel();
			}
			queue.remove(queue.getRequestQueueNodeFor(r));
			if(showAction)
			{
				var url:String = r.op.getUrl();
				var name:String = r.name;
				Main.log.appendMessage(new LogMessage(url, name + "Removed", "Request removed", false, LogMessage.TYPE_END));
			}
			Main.protogeniHandler.dispatchQueueChanged();
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
				else if(msg.search("#2032") > -1)
					failMessage += "\nIO Error, possibly due to server problems or you have no SSL certificate";
			}
			failMessage += "\nURL: " + node.op.getUrl();
			Main.log.appendMessage(new LogMessage(node.op.getUrl(), "Failure", failMessage, true, LogMessage.TYPE_END));
			Main.log.setStatus(node.name + " failed!", true);
			if(!node.continueOnError)
			{
				Main.log.open();
			} else {
				// Find out what to do next
				var next : * = node.fail(event, fault);
				node.cleanup();
				if (next != null)
					queue.push(next);
				
				tryNext();
			}
		}
		
		private function complete(node:Request, code : Number, response : Object) : void
		{
			if(node.removeImmediately)
			{
				node.running = false;
				remove(node, false);
			}
			
			var next;
			try
			{
				// Output completed
				if(code != CommunicationUtil.GENIRESPONSE_SUCCESS)
				{
					Main.log.setStatus(node.name + " done", true);
					Main.log.appendMessage(new LogMessage(node.op.getUrl(), CommunicationUtil.GeniresponseToString(code), node.op.getResponseXml(), true, LogMessage.TYPE_END));
				} else {
					Main.log.setStatus(node.name + " done", false);
					Main.log.appendMessage(new LogMessage(node.op.getUrl(), "Response: " + node.name, node.op.getResponseXml(), false, LogMessage.TYPE_END));
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
		
		public function codeFailure(name:String, detail:String = "", e:Error = null, stop:Boolean = true) : void
		{
			if(stop)
			{
				Main.log.open();
				forceStop = true;
			}
				
			if(e != null)
				Main.log.appendMessage(new LogMessage("", "Code Failure: " + name,detail + "\n\n" + e.toString() + "\n\n" + e.getStackTrace(),true,LogMessage.TYPE_END));
			else
				Main.log.appendMessage(new LogMessage("", "Code Failure: " + name,detail,true,LogMessage.TYPE_END));
			
		}
	}
}