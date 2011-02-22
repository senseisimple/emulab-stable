package
{
	import protogeni.display.ConsoleWindow;
	
	public class LogHandler
	{
		public function LogHandler()
		{
		}
		
		public function appendMessage(msg:LogMessage):void {
			logs.push(msg);
			Main.geniDispatcher.dispatchLogsChanged(msg);
		}
		
		public function viewConsole():void {
			if(console == null)
				console = new ConsoleWindow();
			if(!console.isPopUp)
				console.showWindow();
		}
		
		public function viewGroup(s:String):void {
			viewConsole();
			console.openGroup(s);
		}
		
		public function closeConsole():void {
			console.closeWindow();
			console = null;
		}
		
		public var logs:Array = new Array();
		private var console:ConsoleWindow = null;
	}
}