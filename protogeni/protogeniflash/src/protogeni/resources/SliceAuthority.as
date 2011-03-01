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
		[Bindable]
		public var workingCertGet:Boolean = false;
		
		public function SliceAuthority(newName:String = "", newUrn:String = "", newUrl:String = "", newWorkingCertGet:Boolean = false)
		{
			Name = newName;
			Urn = newUrn;
			if(Urn.length > 0)
				Authority = Urn.split("+")[1];
			Url = newUrl;
			workingCertGet = newWorkingCertGet;
		}
	}
}