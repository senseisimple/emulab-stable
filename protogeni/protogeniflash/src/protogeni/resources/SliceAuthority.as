package protogeni.resources
{
	public class SliceAuthority
	{
		[Bindable]
		public var Name:String;
		public var Urn:String;
		public var Url:String;
		[Bindable]
		public var Authority:String;
		
		public function SliceAuthority(newName:String = "", newUrn:String = "", newUrl:String = "")
		{
			Name = newName;
			Urn = newUrn;
			if(Urn.length > 0)
				Authority = Urn.split("+")[1];
			Url = newUrl;
		}
	}
}