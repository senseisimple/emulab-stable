package protogeni
{
	import flash.events.EventDispatcher;
	import flash.events.IEventDispatcher;
	
	import protogeni.resources.GeniManager;
	import protogeni.resources.Slice;
	
	public class GeniDispatcher extends EventDispatcher
	{
		public function GeniDispatcher(target:IEventDispatcher=null)
		{
			super(target);
		}
		
		// EVENTS
		public function dispatchGeniManagerChanged(gm:GeniManager):void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIMANAGER_CHANGED, gm));
		}
		
		public function dispatchGeniManagersChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.GENIMANAGERS_CHANGED));
		}
		
		public function dispatchQueueChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.QUEUE_CHANGED));
		}
		
		public function dispatchUserChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.USER_CHANGED));
		}
		
		public function dispatchSliceChanged(s:Slice):void {
			dispatchEvent(new GeniEvent(GeniEvent.SLICE_CHANGED, s));
		}
		
		public function dispatchSlicesChanged():void {
			dispatchEvent(new GeniEvent(GeniEvent.SLICES_CHANGED));
		}
	}
}