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
		public var working:Boolean = false;
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
			Main.protogeniHandler.CurrentUser.slices.addOrReplace(slice);
			for each(var sliver:Sliver in slice.slivers)
			{
				if(sliver.created)
					pushRequest(new RequestSliverUpdate(sliver));
				else
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
				//if(sliver.created)
				//pushRequest(new RequestSliverUpdate(sliver, true));
				//else
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
				if (!working && forceStart)
				{
					start();
				}
			}
		}
		
		public function start() : void
		{
			isPaused = false;
			if(working)
				return;
			if (!queue.isEmpty())
			{
				var front:Request = queue.front();
				var op:Operation = front.start();
				op.call(complete, failure);
				Main.log.setStatus(front.name, true, false);
				Main.log.appendMessage(new LogMessage(op.getUrl(), "Send: " + front.name, op.getSendXml(), false, LogMessage.TYPE_START));
				working = true;
			}
			else
			{
				working = false;
			}
			Main.protogeniHandler.dispatchQueueChanged();
		}
		
		public function pause():void
		{
			isPaused = true;
		}
		
		public function remove(rqn:RequestQueueNode):void
		{
			if(working && queue.head == rqn)
			{
				working = false;
				Main.log.setStatus(rqn.item.name + " canceled!", false, false);
			}
			if(queue.contains(rqn))
			{
				rqn.item.cancel();
				var url:String = (rqn.item as Request).op.getUrl();
				var name:String = rqn.item.name;
				queue.remove(rqn);
				Main.protogeniHandler.dispatchQueueChanged();
				Main.log.appendMessage(new LogMessage(url, name + "Canceled", "User canceled", false, LogMessage.TYPE_END));
			}
		}
		
		private function failure(event : ErrorEvent, fault : MethodFault) : void
		{
			working = false;
			// Get and give general info for the failure
			var failMessage:String = "";
			var msg : String = "";
			if (fault != null)
			{
				msg = fault.getFaultString();
				failMessage += "\nFAILURE fault: " + queue.front().name + ": " + msg;
			}
			else
			{
				msg = event.toString();
				failMessage += "\nFAILURE event: " + queue.front().name + ": " + msg;
				if(msg.search("#2048") > -1)
					failMessage += "\nStream error, possibly due to server error";
				else if(msg.search("#2032") > -1)
					failMessage += "\nIO Error, possibly due to server problems or you have no SSL certificate";
			}
			failMessage += "\nURL: " + queue.front().op.getUrl();
			Main.log.appendMessage(new LogMessage(queue.front().op.getUrl(), "Failure", failMessage, true, LogMessage.TYPE_END));
			Main.log.setStatus(queue.front().name + " failed!", false, true);
			if(!queue.front().continueOnError)
			{
				Main.log.open();
			} else {
				// Find out what to do next
				var next : * = queue.front().fail(event, fault);
				queue.front().cleanup();
				queue.pop();
				if (next != null)
					queue.push(next);
				
				tryNext();
			}
		}
		
		private function complete(code : Number, response : Object) : void
		{
			working = false;
			try
			{
				// Output completed
				if(code != CommunicationUtil.GENIRESPONSE_SUCCESS)
				{
					Main.log.setStatus(queue.front().name + " done", false, true);
					Main.log.appendMessage(new LogMessage(queue.front().op.getUrl(), CommunicationUtil.GeniresponseToString(code), queue.front().op.getResponseXml(), true, LogMessage.TYPE_END));
				} else {
					Main.log.setStatus(queue.front().name + " done", false, false);
					Main.log.appendMessage(new LogMessage(queue.front().op.getUrl(), "Response: " + queue.front().name, queue.front().op.getResponseXml(), false, LogMessage.TYPE_END));
				}

				// Find out what to do next
				var next : * = queue.front().complete(code, response);
				var shouldImmediatelyExit:Boolean = queue.head != null && queue.front().waitOnComplete;
				if (next != null)
					queue.push(next);
				queue.front().cleanup();
				queue.pop();
				
				if(shouldImmediatelyExit)
					return;
			}
			catch (e : Error)
			{
				codeFailure(queue.front().name, "Error caught in RPC-Handler Complete", e);
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
		
		public function codeFailure(name:String, detail:String = "", e:Error = null) : void
		{
			Main.log.open();
			if(e != null)
				Main.log.appendMessage(new LogMessage("", "Code Failure: " + name,detail + "\n\n" + e.toString() + "\n\n" + e.getStackTrace(),true,LogMessage.TYPE_END));
			else
				Main.log.appendMessage(new LogMessage("", "Code Failure: " + name,detail,true,LogMessage.TYPE_END));
			forceStop = true;
		}
	}
}