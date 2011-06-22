package protogeni.resources
{
	public class Key
	{
		public var type:String;
		public var value:String;
		
		public function Key(newValue:String, newType:String = "ssh")
		{
			value = newValue;
			type = newType;
		}
	}
}