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
	// Physical link between nodes
	public class PhysicalLink
	{
		public var owner:PhysicalLinkGroup;
		
		[Bindable]
		public var name:String;
		[Bindable]
		public var id:String;
		[Bindable]
		public var manager:GeniManager;
		[Bindable]
		public var linkTypes:Vector.<String> = new Vector.<String>();
		public var interfaceRefs:PhysicalNodeInterfaceCollection = new PhysicalNodeInterfaceCollection();
		
		public var rspec:XML;
		
		// TODEPRECIATE
		[Bindable]
		public var capacity:Number;
		[Bindable]
		public var latency:Number;
		[Bindable]
		public var packetLoss:Number;
		
		public function PhysicalLink(own:PhysicalLinkGroup)
		{
			owner = own;
		}
		
		public function GetNodes():Vector.<PhysicalNode> {
			return interfaceRefs.Nodes();
		}
	}
}