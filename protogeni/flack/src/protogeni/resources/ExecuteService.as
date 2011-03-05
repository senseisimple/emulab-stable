package protogeni.resources
{
	public class ExecuteService
	{
		public var shell:String;
		public var command:String;
		
		public function ExecuteService(newCommand:String = "", newShell:String = "sh")
		{
			shell = newShell;
			command = newCommand;
		}
	}
}