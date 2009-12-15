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
		public static var READY : String = "ready";
	    public static var NOTREADY : String = "notready";
	    public static var FAILED : String = "failed";
	    
		public var uuid : String = null;
		public var hrn : String = null;
		public var urn : String = null;
		public var creator : User = null;
		public var credential : String = "";
		public var slivers : ArrayCollection = new ArrayCollection();
		
		public var status : String = "";

		public function Slice()
		{
		}
		
		public function ReadyIcon():Class {
			switch(status) {
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
				
			return returnString + " (" + status + ")";
		}
		
		public function DetectStatus():String
		{
			if(slivers.length == 0)
			{
				status = "Empty";
				return status;
			}
			var partialReady:Boolean = false;
			var partialFailed:Boolean = false;
			var partialNotready:Boolean = false;
			var allReady:Boolean = true;
			for each(var s:Sliver in slivers)
			{
				if(s.sliceStatus == Slice.READY)
					partialReady = true;
				else if(s.sliceStatus == Slice.FAILED)
				{
					partialFailed = true;
					allReady = false;
				} else if(s.sliceStatus == Slice.NOTREADY)
				{
					partialNotready = true;
					allReady = false;
				} else if(s.sliceStatus.length == 0)
					allReady = false;
			}
			
			var returnString:String = "";
			
			if(allReady)
				returnString += "Ready";
			else
			{
				if(partialReady || partialNotready || partialFailed)
				{
					returnString += "Partially "
					if(partialReady)
					{
						returnString += "ready"
						if(partialNotready || partialFailed)
							returnString += ", ";
					}
					if(partialNotready)
					{
						returnString += "not ready";
						if(partialFailed)
							returnString += ", ";
					}
					if(partialFailed)
						returnString += "failed"
				} else
					returnString = "N/A";
			}
			status = returnString;
			return returnString;
		}
		
		// Used to push more important slices to the top of lists
		public function CompareValue():int {
			
			if(hrn == null && uuid == null) {
				return -1;
			}
			
			if(status == "Ready")
				return 0;
			else if(status.search("Partially") > -1)
				return 1;
			else if(status == "N/A")
				return 2;
			else if(status == "Empty")
				return 3;
			else
				return 4;
		}
	}
}