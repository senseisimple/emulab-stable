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
	// Collection of interfaces from a node in a sliver/slice
	public final class VirtualInterfaceCollection
	{
		[Bindable]
		public var collection:Vector.<VirtualInterface>;
		public function VirtualInterfaceCollection()
		{
			collection = new Vector.<VirtualInterface>();
		}
		
		public function add(ni:VirtualInterface):void {
			for each(var t:VirtualInterface in this.collection)
			{
				if(t.id == ni.id)
					return;
			}
			this.collection.push(ni);
		}
		
		public function remove(vi:VirtualInterface):void
		{
			var idx:int = this.collection.indexOf(vi);
			if(idx > -1)
				this.collection.splice(idx, 1);
		}
		
		public function contains(vi:VirtualInterface):Boolean
		{
			return this.collection.indexOf(vi) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function GetByID(urn:String):VirtualInterface {
			for each(var ni:VirtualInterface in this.collection) {
				if(ni.id == urn)
					return ni;
			}
			return null;
		}
		
		public function Links():Vector.<VirtualLink> {
			var ac:Vector.<VirtualLink> = new Vector.<VirtualLink>();
			for each(var ni:VirtualInterface in this.collection) {
				for each(var l:VirtualLink in ni.virtualLinks.collection) {
					if(ac.indexOf(l) == -1)
						ac.push(l);
				}
			}
			return ac;
		}
		
		public function Nodes():Vector.<VirtualNode> {
			var ac:Vector.<VirtualNode> = new Vector.<VirtualNode>();
			for each(var ni:VirtualInterface in this.collection) {
				if(ac.indexOf(ni.owner) == -1)
					ac.push(ni.owner);
			}
			return ac;
		}
	}
}