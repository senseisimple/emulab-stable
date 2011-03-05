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
	
	// Physical link between nodes
	public class PhysicalLink
	{
		public function PhysicalLink(own:PhysicalLinkGroup)
		{
			owner = own;
		}
		
		public var owner:PhysicalLinkGroup;
		
		[Bindable]
		public var name:String;
		
		[Bindable]
		public var managerString:String;
		
		[Bindable]
		public var manager:GeniManager;
		
		[Bindable]
		public var urn:String;
		
		[Bindable]
		public var interface1:PhysicalNodeInterface;
		
		[Bindable]
		public var interface2:PhysicalNodeInterface;
		
		[Bindable]
		public var bandwidth:Number;
		
		[Bindable]
		public var latency:Number;
		
		[Bindable]
		public var packetLoss:Number;
		
		[Bindable]
		public var types:Vector.<String> = new Vector.<String>();

		public var rspec:XML;
		
		public function GetNodes():Vector.<PhysicalNode> {
			var ac:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
			ac.push(interface1.owner);
			ac.push(interface2.owner);
			return ac;
		}
	}
}