/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
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

      buttons = new ButtonList([clip.okButton], [clickOk]);

      clip.sliceName.text = "demoslice";
      clip.sliceName.setSelection(0, clip.sliceName.length);
      clip.sliceName.alwaysShowSelection = true;
      clip.stage.focus = clip.sliceName;
      clip.sliceName.restrict = "a-zA-Z0-9";

      clip.stage.addEventListener(KeyboardEvent.KEY_UP, keyUp);
    }
	
	public function setSliceName(newSliceName:String):void
	{
		clip.sliceName.text = newSliceName;
      	clip.sliceName.setSelection(0, clip.sliceName.length);
      	clip.stage.focus = clip.sliceName;
	}

    override public function cleanup() : void
    {
      clip.stage.removeEventListener(KeyboardEvent.KEY_UP, keyUp);

      buttons.cleanup();
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
    var buttons : ButtonList;
  }
}
