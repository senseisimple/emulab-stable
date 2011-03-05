package protogeni.resources
{
	public class InstallService
	{
		public var url:String;
		public var installPath:String;
		public var fileType:String;
		
		public function InstallService(newUrl:String = "", newInstallPath:String = "/", newFileType:String = "tar.gz")
		{
			url = newUrl;
			installPath = newInstallPath;
			fileType = newFileType;
		}
	}
}