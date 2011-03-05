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
	public class ProtogeniComponentManager extends GeniManager
	{
		public static var LEVEL_MINIMAL:int = 0;
		public static var LEVEL_FULL:int = 1;
		
		public var Level:int;
		
		[Bindable]
		public var outputRspecVersion:Number;
		[Bindable]
		public var inputRspecVersion:Number;
		
		[Bindable]
		public var outputRspecDefaultVersion:Number;
		
		public var inputRspecVersions:Vector.<Number>;
	    
		public function ProtogeniComponentManager()
		{
			super();
			this.rspecProcessor = new ProtogeniRspecProcessor(this);
			this.type = GeniManager.TYPE_PROTOGENI;
		}
	}
}