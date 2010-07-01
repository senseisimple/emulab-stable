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
	
	// Link as part of a sliver/slice connecting virtual nodes
	public class VirtualLink
	{
		public function VirtualLink(owner:Sliver)
		{
			sliver = owner;
		}
		
		[Bindable]
		public var virtualId:String;
		
		[Bindable]
		public var sliverUrn:String;
		
		public var type:String;
		
		[Bindable]
		public var interfaces:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var bandwidth:Number;
		
		public var sliver:Sliver;

		public var rspec:XML;
		
		public function hasVirtualNode(node:VirtualNode):Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.virtualNode == node)
					return true;
			}
			return false;
		}
		
		public function hasPhysicalNode(node:PhysicalNode):Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.virtualNode.physicalNode == node)
					return true;
			}
			return false;
		}
	}
}