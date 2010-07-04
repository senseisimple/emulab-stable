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
	
	// Interface on a physical node
	public class PhysicalNodeInterface
	{
		public static var CONTROL:int = 0;
		public static var EXPERIMENTAL:int = 1;
		public static var UNUSED:int = 2;
		public static var UNUSED_CONTROL:int = 3;
		public static var UNUSED_EXPERIMENTAL:int = 4;
		
		public function PhysicalNodeInterface(own:PhysicalNode)
		{
			owner = own;
		}
		
		[Bindable]
		public var owner:PhysicalNode;
		
		[Bindable]
		public var id:String;
		
		public var role : int = -1;

		[Bindable]
		public var links:ArrayCollection = new ArrayCollection();
		
		
	}
}