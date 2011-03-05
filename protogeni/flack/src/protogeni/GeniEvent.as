package protogeni
{
	import flash.events.Event;
	
	public class GeniEvent extends Event
	{
		public static const ACTION_CHANGED:int = 0;
		public static const ACTION_REMOVING:int = 1;
		public static const ACTION_CREATED:int = 2;
		public static const ACTION_POPULATED:int = 3;
		
		public static const GENIMANAGER_CHANGED:String = "genimanager_changed";
		public static const GENIMANAGERS_CHANGED:String = "genimanagers_changed";
		public static const QUEUE_CHANGED:String = "queue_changed";
		public static const USER_CHANGED:String = "user_changed";
		public static const SLICE_CHANGED:String = "slice_changed";
		public static const SLIVER_CHANGED:String = "sliver_changed";
		public static const SLICES_CHANGED:String = "slices_changed";
		public static const LOGS_CHANGED:String = "logs_changed";
		
		public function GeniEvent(type:String, object:Object = null, newAction:int = 0)
		{
			super(type);
			changedObject = object;
			action = newAction;
		}
		
		public var changedObject:Object = null;
		public var action:int;
	}
}