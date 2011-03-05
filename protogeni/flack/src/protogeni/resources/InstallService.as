package protogeni.resources
{
	public class InstallService
	{
		public var url:String;
		public var installPath:String;
		
		public function InstallService(newUrl:String = "", newInstallPath:String = "")
		{
			url = newUrl;
			installPath = newInstallPath;
		}
	}
}