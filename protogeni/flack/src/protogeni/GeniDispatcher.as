package protogeni
{
	import flash.events.EventDispatcher;
	import flash.events.IEventDispatcher;
	
	import protogeni.resources.GeniManager;
	import protogeni.resources.GeniUser;
	import protogeni.resources.Slice;
	
	public final class GeniDispatcher extends EventDispatcher
	{
		public function GeniDispatcher(target:IEventDispatcher=null)
		{
			super(target);
		}
		
		// EVENTS
		public function dispatchGeniManagerChanged(gm:GeniManager, action:int = 0):void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIMANAGER_CHANGED, gm, action));
		}
		
		public function dispatchGeniManagersChanged(action:int = 0):void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIMANAGERS_CHANGED, null, action));
		}
		
		public function dispatchQueueChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.QUEUE_CHANGED));
		}
		
		public function dispatchUserChanged():void {
			var u:GeniUser = null;
			if(Main.geniHandler != null)
				u = Main.geniHandler.CurrentUser;
			dispatchEvent(new GeniEvent(GeniEvent.USER_CHANGED, u));
		}
		
		public function dispatchSliceChanged(s:Slice):void {
			dispatchEvent(new GeniEvent(GeniEvent.SLICE_CHANGED, s));
		}
		
		public function dispatchSlicesChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.SLICES_CHANGED));
		}
		
		public function dispatchLogsChanged(m:LogMessage = null):void {
			dispatchEvent(new GeniEvent(GeniEvent.LOGS_CHANGED, m));
		}
	}
}