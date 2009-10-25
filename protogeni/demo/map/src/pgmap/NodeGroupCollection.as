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
	import mx.collections.ArrayCollection;
	
	public class NodeGroupCollection
	{
		public function NodeGroupCollection()
		{
		}
		
		public var collection:ArrayCollection = new ArrayCollection();
		
		public function GetByLocation(lat:Number, lng:Number):NodeGroup {
			for each(var ng:NodeGroup in collection) {
				if(ng.latitude == lat && ng.longitude == lng)
					return ng;
			}
			return null;
		}
		
		public function GetByUrn(urn:String):Node {
			for each(var ng:NodeGroup in collection) {
				var n:Node = ng.GetByUrn(urn);
				if(n != null)
					return n;
			}
			return null;
		}
		
		public function GetByName(name:String):Node {
			for each(var ng:NodeGroup in collection) {
				var n:Node = ng.GetByName(name);
				if(n != null)
					return n;
			}
			return null;
		}
		
		public function GetInterfaceByID(id:String):NodeInterface {
			for each(var ng:NodeGroup in collection) {
				for each(var n:Node in ng.collection) {
					var ni:NodeInterface = n.interfaces.GetByID(id);
					if(ni != null)
						return ni;
				}
			}
			return null;
		}
		
		public function Add(g:NodeGroup):void {
			collection.addItem(g);
		}
	}
}