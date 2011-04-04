package protogeni
{
	import flash.events.Event;
	
	public class ProtogeniEvent extends Event
	{
		public static var COMPONENTMANAGER_CHANGED:String = "componentmanager_changed";
		public static var COMPONENTMANAGERS_CHANGED:String = "componentmanagers_changed";
		public static var QUEUE_CHANGED:String = "queue_changed";
		public static var USER_CHANGED:String = "user_changed";
		public static var SLICE_CHANGED:String = "slice_changed";
		public static var SLIVER_CHANGED:String = "sliver_changed";
		public static var SLICES_CHANGED:String = "slices_changed";
		
		public function ProtogeniEvent(type:String, object:Object = null)
		{
			super(type);
			changedObject = object;
		}
		
		public var changedObject:Object = null;
	}
}