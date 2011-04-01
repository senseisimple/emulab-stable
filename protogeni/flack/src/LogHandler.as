package
{
	import protogeni.GeniEvent;
	import protogeni.display.ConsoleWindow;
	
	public class LogHandler
	{
		public static function appendMessage(msg:LogMessage):void {
			logs.push(msg);
			Main.geniDispatcher.dispatchLogsChanged(msg);
		}
		
		public static function viewConsole():void {
			if(console == null)
				console = new ConsoleWindow();
			console.showWindow();
		}
		
		public static function closeConsole():void {
			console.closeWindow();
			console = null;
		}
		
		public static function viewGroup(s:String):void {
			viewConsole();
			console.openGroup(s);
		}
		
		public static function clear():void {
			logs = new Vector.<LogMessage>();
			Main.geniDispatcher.dispatchLogsChanged();
		}
		
		public static var logs:Vector.<LogMessage> = new Vector.<LogMessage>();
		private static var console:ConsoleWindow = null;
	}
}