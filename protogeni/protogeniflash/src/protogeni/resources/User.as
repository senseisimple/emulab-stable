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
	
	// ProtoGENI user information
	public class User
	{
		[Bindable]
		public var uid:String;
		
		[Bindable]
		public var uuid:String;
		[Bindable]
		public var hrn:String;
		[Bindable]
		public var email:String;
		[Bindable]
		public var name:String;
		public var credential:String;
		public var keys:Array;
		[Bindable]
		public var urn:String;
		
		public var slices:SliceCollection;
		
		public function User()
		{
			slices = new SliceCollection();
		}
	}
}