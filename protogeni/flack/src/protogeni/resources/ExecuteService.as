package protogeni.resources
{
	public class ExecuteService
	{
		public var shell:String;
		public var command:String;
		
		public function ExecuteService(newShell:String = "", newCommand:String = "")
		{
			shell = newShell;
			command = newCommand;
		}
	}
}