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
  public class Interface
  {
    public function Interface(newName : String) : void
    {
      name = newName;
      used = false;
      role = CONTROL;
    }

    public function clone() : Interface
    {
      var result = new Interface(name);
      result.used = used;
      result.role = role;
      return result;
    }

    public var name : String;
    public var used : Boolean;
    public var role : int;

    public static var CONTROL = 0;
    public static var EXPERIMENTAL = 1;
  }
}
