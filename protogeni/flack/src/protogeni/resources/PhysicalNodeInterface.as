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
	
	// Interface on a physical node
	public class PhysicalNodeInterface
	{
		public static const ROLE_CONTROL:int = 0;
		public static const ROLE_EXPERIMENTAL:int = 1;
		public static const ROLE_UNUSED:int = 2;
		public static const ROLE_UNUSED_CONTROL:int = 3;
		public static const ROLE_UNUSED_EXPERIMENTAL:int = 4;
		public static function RoleStringFromInt(i:int):String
		{
			switch(i) {
				case ROLE_CONTROL: return "control";
				case ROLE_EXPERIMENTAL: return "experimental";
				case ROLE_UNUSED: return "unused";
				case ROLE_UNUSED_CONTROL: return "unused_control";
				case ROLE_UNUSED_EXPERIMENTAL: return "unused_experimental";
				default: return "";
			}
		}
		public static function RoleIntFromString(s:String):int
		{
			switch(s) {
				case "control": return ROLE_CONTROL;
				case "experimental": return ROLE_EXPERIMENTAL;
				case "unused": return ROLE_UNUSED;
				case "unused_control": return ROLE_UNUSED_CONTROL;
				case "unused_experimental": return ROLE_UNUSED_EXPERIMENTAL;
				default: return -1;
			}
		}
		
		[Bindable]
		public var owner:PhysicalNode;
		
		[Bindable]
		public var id:String;
		public var role : int = -1;
		public var publicIPv4:String = "";
		
		[Bindable]
		public var physicalLinks:Vector.<PhysicalLink> = new Vector.<PhysicalLink>();
		
		public function PhysicalNodeInterface(own:PhysicalNode)
		{
			owner = own;
		}
	}
}