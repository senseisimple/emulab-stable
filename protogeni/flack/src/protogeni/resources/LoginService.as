package protogeni.resources
{
	public class LoginService
	{
		public var authentication:String;
		public var hostname:String;
		public var port:String;
		public var username:String;
		
		public function LoginService(newAuth:String = "",
										newHost:String = "",
										newPort:String = "",
										newUser:String = "")
		{
			authentication = newAuth;
			hostname = newHost;
			port = newPort;
			username = newUser;
		}
	}
}