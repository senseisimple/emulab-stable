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
	
	public class VirtualNode
	{
		public function VirtualNode(owner:Sliver)
		{
			sliver = owner;
		}
		
		public var physicalNode:PhysicalNode;
		public var virtualId:String;
		public var virtualizationType:String;
		public var interfaces:ArrayCollection = new ArrayCollection();
		public var rspec:XML;
		public var sliver:Sliver;
		public var hostname:String;
		//public var uuid:String;
		//public var urn:String;
		//public var status : String = "N/A";
	}
}