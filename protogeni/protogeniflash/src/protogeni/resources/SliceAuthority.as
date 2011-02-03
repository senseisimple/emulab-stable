package protogeni.resources
{
	public class SliceAuthority
	{
		[Bindable]
		public var Name:String;
		public var Urn:String;
		public var Url:String;
		
		public function SliceAuthority(newName:String = "", newUrn:String = "", newUrl:String = "")
		{
			Name = newName;
			Urn = newUrn;
			Url = newUrl;
		}
	}
}