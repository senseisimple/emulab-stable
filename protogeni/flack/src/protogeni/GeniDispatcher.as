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
	import flash.events.EventDispatcher;
	import flash.events.IEventDispatcher;
	
	import protogeni.resources.GeniManager;
	import protogeni.resources.GeniUser;
	import protogeni.resources.Slice;
	
	/**
	 * Dispatches events to any listener for GENI-related events
	 * 
	 * @author mstrum
	 * 
	 */
	public final class GeniDispatcher extends EventDispatcher
	{
		public function GeniDispatcher(target:IEventDispatcher = null)
		{
			super(target);
		}
		
		// EVENTS
		public function dispatchGeniManagerChanged(gm:GeniManager,
												   action:int = 0):void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIMANAGER_CHANGED,
										gm,
										action));
		}
		
		public function dispatchGeniManagersChanged(action:int = 0):void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIMANAGERS_CHANGED,
										null,
										action));
		}
		
		public function dispatchQueueChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.QUEUE_CHANGED));
		}
		
		public function dispatchUserChanged():void {
			var u:GeniUser = null;
			if(Main.geniHandler != null)
				u = Main.geniHandler.CurrentUser;
			dispatchEvent(new GeniEvent(GeniEvent.USER_CHANGED,
										u));
		}
		
		public function dispatchSliceChanged(s:Slice, action:int = 0):void {
			dispatchEvent(new GeniEvent(GeniEvent.SLICE_CHANGED,
										s,
										action));
		}
		
		public function dispatchSlicesChanged(action:int = 0):void {
			dispatchEvent(new GeniEvent(GeniEvent.SLICES_CHANGED,
				Main.geniHandler.CurrentUser.slices,
				action));
		}
		
		public function dispatchLogsChanged(m:LogMessage = null):void {
			dispatchEvent(new GeniEvent(GeniEvent.LOGS_CHANGED,
										m));
		}
		
		public function dispatchGeniAuthoritiesChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIAUTHORITIES_CHANGED));
		}
	}
}