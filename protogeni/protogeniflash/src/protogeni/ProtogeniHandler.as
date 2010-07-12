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
 
 package protogeni
{
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.events.IEventDispatcher;
	
	import mx.collections.ArrayCollection;
	
	import protogeni.communication.ProtogeniRpcHandler;
	import protogeni.communication.RequestDiscoverResources;
	import protogeni.communication.RequestGetCredential;
	import protogeni.communication.RequestGetKeys;
	import protogeni.communication.RequestListComponents;
	import protogeni.display.DisplayUtil;
	import protogeni.display.ProtogeniMapHandler;
	import protogeni.resources.ComponentManager;
	import protogeni.resources.ComponentManagerCollection;
	import protogeni.resources.User;
	
	// Holds and handles all information regarding ProtoGENI
	public class ProtogeniHandler extends EventDispatcher
	{
		
		[Bindable]
		public var rpcHandler : ProtogeniRpcHandler;
		
		[Bindable]
		public var mapHandler : ProtogeniMapHandler;
		
		[Bindable]
		public var CurrentUser:User;
		
		public var ComponentManagers:ComponentManagerCollection;

		public function ProtogeniHandler()
		{
			rpcHandler = new ProtogeniRpcHandler();
			mapHandler = new ProtogeniMapHandler();
			addEventListener(COMPONENTMANAGER_CHANGED, mapHandler.drawMap);
			ComponentManagers = new ComponentManagerCollection();
			CurrentUser = new User();
		}
		
		public function clearAll() : void
		{
			ComponentManagers = new ComponentManagerCollection();
		}
		
		// EVENTS
		public static var COMPONENTMANAGER_CHANGED:String = "componentmanager_changed";
		public static var COMPONENTMANAGERS_CHANGED:String = "componentmanagers_changed";
		public static var QUEUE_CHANGED:String = "queue_changed";
		public static var USER_CHANGED:String = "user_changed";
		public static var SLICE_CHANGED:String = "slice_changed";
		public static var SLICES_CHANGED:String = "slices_changed";
		
		public function dispatchComponentManagerChanged():void {
			dispatchEvent(new Event(ProtogeniHandler.COMPONENTMANAGER_CHANGED));
		}
		
		public function dispatchComponentManagersChanged():void {
			dispatchEvent(new Event(ProtogeniHandler.COMPONENTMANAGERS_CHANGED));
		}
		
		public function dispatchQueueChanged():void {
			dispatchEvent(new Event(ProtogeniHandler.QUEUE_CHANGED));
		}
		
		public function dispatchUserChanged():void {
			dispatchEvent(new Event(ProtogeniHandler.USER_CHANGED));
		}
		
		public function dispatchSliceChanged():void {
			dispatchEvent(new Event(ProtogeniHandler.SLICE_CHANGED));
		}
		
		public function dispatchSlicesChanged():void {
			dispatchEvent(new Event(ProtogeniHandler.SLICES_CHANGED));
		}
	}
}