/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
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

package
{
	import flash.net.LocalConnection;
	import flash.events.StatusEvent;
	
	public class ConnectionHandler
	{
		private var sender:LocalConnection;
		private var reciever:LocalConnection;
		
		public function ConnectionHandler()
		{
			sender = new LocalConnection();
			sender.addEventListener(StatusEvent.STATUS, onConnectionStatus);
			//sender.allowDomain("localhost");
			reciever = new LocalConnection();
			//reciever.allowDomain("localhost");
			try
			{
				reciever.connect("protogeni-maptoflash");
				reciever.client = this;
				reciever.addEventListener(StatusEvent.STATUS, onConnectionStatus);
			} catch (err:Error)
			{
				// connection already made, this should probably display a message
			}
		}
		
		public function addnode(urn:String, cm:String):void
		{
			var resp:String = Main.menu.getComponent(urn, cm);
			sender.send("protogeni-flashtomap", "updatestatus", resp);
		}
		
		public function onConnectionStatus(statusEvent:StatusEvent):void
		{
			if(statusEvent.level == "error")
			{
			sender.send("protogeni-flashtomap", "updatestatus", "ah crap, the flash client has a connection status problem");
			}
		}
	}
}