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
			collection.addItem(ni);
		}
		
		public function Links():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var ni:VirtualInterface in collection) {
				for each(var l:VirtualLink in ni.links) {
					ac.addItem(l);
				}
			}
			return ac;
		}
	}
}