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
	
	public class Node
	{
		public function Node(own:NodeGroup)
		{
			owner = own;
		}
		
		public var owner:NodeGroup;
		
		[Bindable]
		public var name:String;
		
		[Bindable]
		public var uuid:String;
		
		[Bindable]
		public var manager:String;
		
		public var available:Boolean;
		public var exclusive:Boolean;
		
		public var slice : Slice = null;
		public var status : String;
		
		[Bindable]
		public var types:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var interfaces:NodeInterfaceCollection = new NodeInterfaceCollection();
		
		public var rspec:XML;

		public function GetLatitude():Number {
			return owner.latitude;
		}

		public function GetLongitude():Number {
			return owner.longitude;
		}
		
		public function GetLinks():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:NodeInterface in interfaces.collection) {
				for each(var l:Link in i.links) {
					ac.addItem(l);
				}
			}
			return ac;
		}
	}
}