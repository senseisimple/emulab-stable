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
	
	public class Link
	{
		public function Link(own:LinkGroup)
		{
			owner = own;
		}
		
		public var owner:LinkGroup;
		
		[Bindable]
		public var name:String;
		
		[Bindable]
		public var manager:String;
		
		[Bindable]
		public var uuid:String;
		
		[Bindable]
		public var urn:String;
		
		[Bindable]
		public var interface1:NodeInterface;
		
		[Bindable]
		public var interface2:NodeInterface;
		
		[Bindable]
		public var bandwidth:Number;
		
		[Bindable]
		public var latency:Number;
		
		[Bindable]
		public var packetLoss:Number;
		
		[Bindable]
		public var types:ArrayCollection = new ArrayCollection();
		
		public var slice : Slice = null;

		public var rspec:XML;
		
		public function GetNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			ac.addItem(interface1.owner);
			ac.addItem(interface2.owner);
			return ac;
		}
	}
}