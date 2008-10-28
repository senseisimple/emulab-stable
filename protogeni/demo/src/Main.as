package
{
  import flash.display.DisplayObjectContainer;

  public class Main
  {
    public static function init(newParent : DisplayObjectContainer)
    {
      parent = newParent;

      menu = new MenuSliceSelect();
      menu.init(parent);
    }

    public static function changeState(newMenu : MenuState)
    {
      menu.cleanup();
      menu = newMenu;
      menu.init(parent);
    }

    static var parent : DisplayObjectContainer;
    static var menu : MenuState;
  }
}
