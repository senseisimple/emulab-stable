package protogeni
{
	import flash.events.Event;
	
	public class GeniEvent extends Event
	{
		public static var GENIMANAGER_CHANGED:String = "genimanager_changed";
		public static var GENIMANAGERS_CHANGED:String = "genimanagers_changed";
		public static var QUEUE_CHANGED:String = "queue_changed";
		public static var USER_CHANGED:String = "user_changed";
		public static var SLICE_CHANGED:String = "slice_changed";
		public static var SLIVER_CHANGED:String = "sliver_changed";
		public static var SLICES_CHANGED:String = "slices_changed";
		public static var LOGS_CHANGED:String = "logs_changed";
		
		public function GeniEvent(type:String, object:Object = null)
		{
			super(type);
			changedObject = object;
		}
		
		public var changedObject:Object = null;
	}
}