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
 
 package protogeni.resources
{
	// Interface on a virtual node
	public class VirtualInterface
	{
		public static var tunnelNext:int = 1;
		public static function getNextTunnel():String
		{
			var first:int = ((tunnelNext >> 8) & 0xff);
			var second:int = (tunnelNext & 0xff);
			tunnelNext++;
			return "192.168." + String(first) + "." + String(second);
		}
		
		[Bindable]
		public var owner:VirtualNode;
		public var physicalNodeInterface:PhysicalNodeInterface;
		
		[Bindable]
		public var id:String;
		
		// tunnel stuff
		public var ip:String = "";
		public var mask:String = ""; // 255.255.255.0
		public var type:String = ""; //ipv4
		
		[Bindable]
		public var virtualLinks:VirtualLinkCollection;
		
		// depreciated
		public var bandwidth:int = 100000;
		
		public function VirtualInterface(own:VirtualNode)
		{
			owner = own;
			virtualLinks = new VirtualLinkCollection();
		}
		
		public function IsBound():Boolean {
			return physicalNodeInterface != null;
		}
	}
}