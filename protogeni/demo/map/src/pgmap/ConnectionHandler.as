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
 
 package pgmap
{
	import flash.events.StatusEvent;
	import flash.net.LocalConnection;
	
	import merapi.Bridge;
	import merapi.handlers.MessageHandler;
	import merapi.messages.IMessage;
	import merapi.messages.Message;
	
	import mx.controls.Alert;
	import mx.rpc.events.ResultEvent;
	
	// Handles communication with the flash client
	public class ConnectionHandler
	{
		private var onlyReciever:Boolean = true;
		private var sender:LocalConnection;
		private var reciever:LocalConnection;
		
		public function ConnectionHandler()
		{
			sender = new LocalConnection();
			sender.allowDomain("*");
			sender.addEventListener(StatusEvent.STATUS, onConnectionStatus);
			reciever = new LocalConnection();
			reciever.allowDomain("*");
			try
			{
				reciever.connect("protogeni-flashtomap");
				reciever.client = this;
				reciever.addEventListener(StatusEvent.STATUS, onConnectionStatus);
			} catch (err:Error)
			{
				Alert.show("Only one map instance can listen to messages from the flash client.  If you need that functionality, return to the first instance.");
				onlyReciever = false;
			}
		}
		
		public function RequestNode(urn:String, cm:String):void
		{
			// Send to flash client
			sender.send("protogeni-maptoflash", "addnode", urn, cm);
			
			// Send to java client
			var message : Message = new Message();
			message.data = urn + " " + cm;
			message.type = "RequestNode";
			Bridge.getInstance().sendMessage( message );
		}
		
		// Feedback from flash client
		public function updatestatus(status:String):void
		{
			Alert.show(status);
		}
		
		// Feedback from java client
		public function handleResult(event:ResultEvent):void {
			var message:IMessage = event.result as IMessage;
			Alert.show(message.data.toString());
		}

		public function onConnectionStatus(statusEvent:StatusEvent):void
		{
			if(statusEvent.level == "error")
			{
				Alert.show("ERROR ON MAP CONNECTION STATUS");
			}
		}
	}
}