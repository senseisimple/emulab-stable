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
	// Collection of interfaces from a physical node
	public class PhysicalNodeInterfaceCollection
	{
		public var collection:Vector.<PhysicalNodeInterface>;
		public function PhysicalNodeInterfaceCollection()
		{
			collection = new Vector.<PhysicalNodeInterface>();
		}
		
		public function add(ni:PhysicalNodeInterface):void {
			collection.push(ni);
		}
		
		public function remove(vi:PhysicalNodeInterface):void
		{
			var idx:int = collection.indexOf(vi);
			if(idx > -1)
				collection.splice(idx, 1);
		}
		
		public function contains(vi:PhysicalNodeInterface):Boolean
		{
			return collection.indexOf(vi) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function GetByID(urn:String, exact:Boolean = true):PhysicalNodeInterface {
			for each(var ni:PhysicalNodeInterface in collection) {
				if(ni.id == urn)
					return ni;
				if(!exact && ni.id.indexOf(urn) != -1)
					return ni;
			}
			return null;
		}
		
		public function Links():Vector.<PhysicalLink> {
			var ac:Vector.<PhysicalLink> = new Vector.<PhysicalLink>();
			for each(var ni:PhysicalNodeInterface in collection) {
				for each(var l:PhysicalLink in ni.physicalLinks) {
					ac.push(l);
				}
			}
			return ac;
		}
		
		public function Nodes():Vector.<PhysicalNode> {
			var ac:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
			for each(var ni:PhysicalNodeInterface in collection) {
				if(ac.indexOf(ni.owner) == -1)
					ac.push(ni.owner);
			}
			return ac;
		}
	}
}