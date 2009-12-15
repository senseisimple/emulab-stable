package
{
	import mx.controls.Alert;
	
  public class Main
  {
    public static function getConsole() : LogRoot
    {
      return log;
    }

    public static function setConole(newLog : LogRoot) : void
    {
      log = newLog;
    }
    
    public static function startTimer() : void
    {
    	d = new Date();
    }

    private static var log : LogRoot = null;
    private static var d : Date = null;
  }
}
