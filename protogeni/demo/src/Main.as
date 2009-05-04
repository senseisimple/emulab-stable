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
  import flash.text.TextField;

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

    public static function getConsole() : TextField
    {
      return menu.getConsole();
    }

    public static function setText(newText : String) : void
    {
      text = newText;
    }

    public static function getText() : String
    {
      return text;
    }

    static var parent : DisplayObjectContainer;
    static var menu : MenuState;

    static var text : String;
  }
}
