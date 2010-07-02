package
{
  import flash.text.TextField;

  public class Main
  {
    public static function execute(choice : MenuClip,
                                   progress : WaitingClip) : void
    {
      menu = new Menu(choice, progress);
    }

    public static function getConsole() : TextField
    {
      return menu.getConsole();
    }

    private static var menu : Menu;
  }
}
