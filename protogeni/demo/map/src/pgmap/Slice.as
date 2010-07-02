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
	
	// Slice that a user created in ProtoGENI
	public class Slice
	{
		// Status values
	    public static var CHANGING : String = "changing";
		public static var READY : String = "ready";
	    public static var NOTREADY : String = "notready";
	    public static var FAILED : String = "failed";
	    public static var UNKNOWN : String = "unknown";
	    public static var NA : String = "N/A";
	    
	    // State values
	    public static var STARTED : String = "started";
		public static var STOPPED : String = "stopped";
	    public static var MIXED : String = "mixed";
	    
		public var uuid : String = null;
		public var hrn : String = null;
		public var urn : String = null;
		public var creator : User = null;
		public var credential : String = "";
		public var slivers : ArrayCollection = new ArrayCollection();

		public function Slice()
		{
		}
		
		public function Status():String {
			if(hrn == null) return null;
			var status:String = NA;
			for each(var sliver:Sliver in slivers) {
				if(status == NA) status = sliver.status;
				else {
					if(sliver.status != status)
						return "mixed";
				}
			}
			return status;
		}
		
		public function ReadyIcon():Class {
			switch(Status()) {
				case READY : return Common.flagGreenIcon;
				case NOTREADY : return Common.flagYellowIcon;
				case FAILED : return Common.flagRedIcon;
				default : return null;
			}
		}
		
		public function DisplayString():String {
			
			if(hrn == null && uuid == null) {
				return "All Resources";
			}
			
			var returnString : String;
			if(hrn == null)
				returnString = uuid;
			else
				returnString = hrn;
				
			return returnString + " (" + Status() + ")";
		}
		
		// Used to push more important slices to the top of lists
		public function CompareValue():int {
			
			if(hrn == null && uuid == null) {
				return -1;
			}
			
			if(Status() == "ready")
				return 0;
			else
				return 1;
		}
	}
}