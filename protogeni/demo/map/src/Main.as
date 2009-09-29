package
{
	import mx.controls.Alert;
	
  public class Main
  {
    public function getConsole() : LogRoot
    {
      return log;
    }

    public function setConole(newLog : LogRoot) : void
    {
      log = newLog;
    }
    
    public function startTimer() : void
    {
    	d = new Date();
    }
    
    public function endTimer() : void
    {
    	var d2 : Date = new Date();
    	Alert.show((d.valueOf() - d2.valueOf()).toString());
    }

    private static var log : LogRoot = null;
    private static var d : Date = null;
  }
}
