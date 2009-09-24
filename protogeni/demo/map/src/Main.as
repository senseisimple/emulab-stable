package
{
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

    private static var log : LogRoot = null;
  }
}
