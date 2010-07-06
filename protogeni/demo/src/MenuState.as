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

  class MenuState
  {
    public function MenuState() : void
    {
    }

    public function init(parent : DisplayObjectContainer) : void
    {
    }

    public function cleanup() : void
    {
    }

    public function getConsole() : TextField
    {
      return null;
    }
	
	public function getComponent(urn:String, cm:String, force:Boolean = false):String
	{
		return "The flash client is not ready to process messages.  Please make sure the slice is created.";
	}
  }
}
