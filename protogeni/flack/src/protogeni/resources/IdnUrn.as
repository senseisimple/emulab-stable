package protogeni.resources
{
	public class IdnUrn
	{
		[Bindable]
		public var full:String;
		
		public function get authority():String {
			if(full.length > 0)
				return full.split("+")[1];
			else
				return "";
		}
		
		public function get type():String {
			if(full.length > 0)
				return full.split("+")[2];
			else
				return "";
		}
		
		public function get name():String {
			if(full.length > 0)
				return full.split("+")[3];
			else
				return "";
		}
		
		public function IdnUrn(urn:String = "")
		{
			full = urn;
		}
		
		public static function getAuthorityFrom(urn:String) : String
		{
			return urn.split("+")[1];
		}
		
		public static function getTypeFrom(urn:String) : String
		{
			return urn.split("+")[2];
		}
		
		public static function getNameFrom(urn:String) : String
		{
			return urn.split("+")[3];
		}
		
		public static function makeFrom(newAuthority:String, newType:String, newName:String):IdnUrn
		{
			return new IdnUrn("urn:publicid:IDN+" + newAuthority + "+" + newType + "+" + newName);
		}
	}
}