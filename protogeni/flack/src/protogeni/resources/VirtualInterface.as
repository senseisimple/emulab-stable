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

package protogeni.resources
{
	/**
	 * Interface on a resource used to connect to other resources through links
	 *  
	 * @author mstrum
	 * 
	 */	
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

		public var capacity:int = 100000;
		
		public function VirtualInterface(own:VirtualNode)
		{
			this.owner = own;
			this.virtualLinks = new VirtualLinkCollection();
		}
		
		public function nodesOtherThan(node:VirtualNode = null):VirtualNodeCollection {
			var ignoreNode:VirtualNode = node;
			if(node == null)
				ignoreNode = this.owner;
			var ac:VirtualNodeCollection = new VirtualNodeCollection();
			for each(var virtualLink:VirtualLink in this.virtualLinks.collection) {
				for each(var destInterface:VirtualInterface in virtualLink.interfaces.collection) {
					if(destInterface.owner != ignoreNode
						&& !ac.contains(destInterface.owner))
						ac.add(destInterface.owner);
				}
			}
			return ac;
		}
		
		public function IsBound():Boolean {
			return this.physicalNodeInterface != null;
		}
	}
}