package
{
  import flash.display.DisplayObjectContainer;
  import flash.events.MouseEvent;
  import flash.events.KeyboardEvent;
  import flash.ui.Keyboard;

  class MenuSliceSelect extends MenuState
  {
    public function MenuSliceSelect() : void
    {
    }

    override public function init(newParent : DisplayObjectContainer) : void
    {
      clip = new SliceSelectClip();
      newParent.addChild(clip);

      clip.ok.label = "Create Slice";
      clip.ok.addEventListener(MouseEvent.CLICK, clickOk);

      clip.sliceName.text = "demoslice";
      clip.sliceName.setSelection(0, clip.sliceName.length);
      clip.sliceName.alwaysShowSelection = true;
      clip.stage.focus = clip.sliceName;

      clip.stage.addEventListener(KeyboardEvent.KEY_UP, keyUp);
    }

    override public function cleanup() : void
    {
      clip.ok.removeEventListener(MouseEvent.CLICK, clickOk);
      clip.stage.removeEventListener(KeyboardEvent.KEY_UP, keyUp);

      clip.parent.removeChild(clip);
    }

    function clickOk(event : MouseEvent) : void
    {
      createSlice();
    }

    function keyUp(event : KeyboardEvent) : void
    {
      if (event.keyCode == Keyboard.ENTER)
      {
        createSlice();
      }
    }

    function createSlice() : void
    {
      var newMenu : MenuState = new MenuSliceCreate(clip.sliceName.text);
//      var newMenu : MenuState = new MenuSliceDetail(clip.sliceName.text,
//      "", "");
      Main.changeState(newMenu);
    }

    var clip : SliceSelectClip;
  }
}
