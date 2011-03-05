package
{
	public class LogMessage
	{
		public static var TYPE_OTHER : int = 0;
		public static var TYPE_START : int = 1;
		public static var TYPE_END : int = 2;
		
		public function LogMessage(newGroupId:String = "", newName:String = "", newDetails:String = "", newIsError:Boolean = false, newType:int = 0)
		{
			groupId = newGroupId;
			name = newName;
			details = newDetails;
			isError = newIsError;
			timeStamp = new Date();
			type = newType;
		}
		
		public function toString():String
		{
			return "-MSG-----------------\n" +
				"Name: " + name + "\n" +
				"Group ID: " + groupId + "\n" +
				"Time: " + timeStamp + "\n" +
				"Is Error?: " + isError + "\n" +
				"Details:\n" + details +
				"\n-----------------END-";
		}
		
		public var groupId : String;	// Identifier to group, like what CM it's associated with 
		public var name : String;		// Human-readable name
		public var details : String;	// Full text
		public var isError : Boolean;	// Was this an error?
		public var timeStamp : Date;	// When did this event occur?
		public var type : int;			// What kind of log is this?
	}
}