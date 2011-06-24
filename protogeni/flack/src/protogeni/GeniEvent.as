/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
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

package protogeni
{
	import flash.events.Event;
	
	/**
	 * Event within the GENI world
	 * 
	 * @author mstrum
	 * 
	 */
	public final class GeniEvent extends Event
	{
		public static const ACTION_CHANGED:int = 0;
		public static const ACTION_REMOVING:int = 1;
		public static const ACTION_CREATED:int = 2;
		public static const ACTION_POPULATED:int = 3;
		public static const ACTION_REMOVED:int = 4;
		
		public static const GENIMANAGER_CHANGED:String = "genimanager_changed";
		public static const GENIMANAGERS_CHANGED:String = "genimanagers_changed";
		public static const QUEUE_CHANGED:String = "queue_changed";
		public static const USER_CHANGED:String = "user_changed";
		public static const SLICE_CHANGED:String = "slice_changed";
		public static const SLIVER_CHANGED:String = "sliver_changed";
		public static const SLICES_CHANGED:String = "slices_changed";
		public static const LOGS_CHANGED:String = "logs_changed";
		public static const GENIAUTHORITIES_CHANGED:String = "geniauthorities_changed";
		
		public var changedObject:Object = null;
		public var action:int;
		
		public function GeniEvent(type:String,
								  object:Object = null,
								  newAction:int = 0)
		{
			super(type);
			changedObject = object;
			action = newAction;
		}
	}
}