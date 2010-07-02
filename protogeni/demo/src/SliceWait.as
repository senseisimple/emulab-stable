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
  import flash.events.Event;
  import flash.text.TextField;

  public class SliceWait
  {
    public function SliceWait(newClip : SliceWaitClip,
                              newOpText : TextField,
                              newConsole : Console) : void
    {
      clip = newClip;
      opText = newOpText;
      console = newConsole;
      clip.addEventListener(Event.ENTER_FRAME, enterFrame);
      clip.rotation = Math.random()*360;
    }

    public function cleanup() : void
    {
      clip.removeEventListener(Event.ENTER_FRAME, enterFrame);
    }

    function enterFrame(event : Event) : void
    {
      if (console.isWorking())
      {
        if (! clip.visible)
        {
          clip.visible = true;
          opText.visible = true;
        }
        clip.rotation += 5;
      }
      else
      {
        if (clip.visible)
        {
          clip.visible = false;
          opText.visible = false;
        }
      }
    }

    var clip : SliceWaitClip;
    var opText : TextField;
    var console : Console;
  }
}
