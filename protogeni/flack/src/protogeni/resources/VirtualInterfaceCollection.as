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
	import mx.collections.ArrayCollection;
	
	// Collection of interfaces from a node in a sliver/slice
	public class VirtualInterfaceCollection
	{
		public function VirtualInterfaceCollection()
		{
		}
		
		[Bindable]
		public var collection:ArrayCollection = new ArrayCollection();
		
		public function GetByID(urn:String):VirtualInterface {
			for each(var ni:VirtualInterface in collection) {
				if(ni.id == urn)
					return ni;
			}
			return null;
		}
		
		public function Add(ni:VirtualInterface):void {
			for each(var t:VirtualInterface in collection)
			{
				if(t.id == ni.id)
					return;
			}
			collection.addItem(ni);
		}
		
		public function removeAll():void {
			collection.removeAll();
		}
		
		public function Links():Vector.<VirtualLink> {
			var ac:Vector.<VirtualLink> = new Vector.<VirtualLink>();
			for each(var ni:VirtualInterface in collection) {
				for each(var l:VirtualLink in ni.virtualLinks) {
					if(ac.indexOf(l) == -1)
						ac.push(l);
				}
			}
			return ac;
		}
		
		public function Nodes():Vector.<VirtualNode> {
			var ac:Vector.<VirtualNode> = new Vector.<VirtualNode>();
			for each(var ni:VirtualInterface in collection) {
				if(ac.indexOf(ni.owner) == -1)
					ac.push(ni.owner);
			}
			return ac;
		}
	}
}