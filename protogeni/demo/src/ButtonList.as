/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
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
  import flash.events.MouseEvent;

  public class ButtonList
  {
    public function ButtonList(newButtons : Array, newFunctions : Array) : void
    {
      buttons = newButtons;
      functions = newFunctions;
      var i : int = 0;
      for (; i < buttons.length; ++i)
      {
        buttons[i].addEventListener(MouseEvent.CLICK, functions[i]);
        buttons[i].addEventListener(MouseEvent.MOUSE_OVER, rollOver);
        buttons[i].addEventListener(MouseEvent.MOUSE_OUT, rollOut);
        buttons[i].mouseChildren = false;
      }
    }

    public function cleanup() : void
    {
      var i : int = 0;
      for (; i < buttons.length; ++i)
      {
        buttons[i].removeEventListener(MouseEvent.CLICK, functions[i]);
        buttons[i].removeEventListener(MouseEvent.MOUSE_OVER, rollOver);
        buttons[i].removeEventListener(MouseEvent.MOUSE_OUT, rollOut);
      }
    }

    function rollOver(event : MouseEvent) : void
    {
      var index : int = getIndex(event);
      if (index != -1
          && buttons[index].currentFrame == NORMAL)
      {
        buttons[index].gotoAndStop(OVER);
      }
    }

    function rollOut(event : MouseEvent) : void
    {
      var index : int = getIndex(event);
      if (index != -1
          && buttons[index].currentFrame == OVER)
      {
        buttons[index].gotoAndStop(NORMAL);
      }
    }

    function getIndex(event : MouseEvent) : int
    {
      var index : int = -1;
      var i : int = 0;
      for (; i < buttons.length; ++i)
      {
        if (buttons[i] == event.target)
        {
          index = i;
          break;
        }
      }
      return index;
    }

    var buttons : Array;
    var functions : Array;

    public static var NORMAL : int = 1;
    public static var OVER : int = 2;
    public static var GHOSTED : int = 3;
  }
}
