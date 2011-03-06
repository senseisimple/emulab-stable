/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
* All rights reserved.
*
* Permission to use, copy, modify and distribute this software is hereby
* granted provided that (1) source code retains these copyright, permission,
* and disclaimer notices, and (2) redistributions including binaries
* reproduce the notices in supporting documentation.
*
* THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
* CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
* FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
*/

package
{
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