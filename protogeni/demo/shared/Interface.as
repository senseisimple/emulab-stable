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
    public function Interface(newVirtualId : String, newName : String) : void
    {
      virtualId = newVirtualId;
      name = newName;
      used = false;
      role = UNUSED;
      bandwidth = 0;
    }

    public function clone() : Interface
    {
      var result = new Interface(virtualId, name);
      result.used = used;
      result.role = role;
      result.bandwidth = bandwidth;
      return result;
    }

    public var virtualId : String;
    public var name : String;
    public var used : Boolean;
    public var role : int;
    public var bandwidth : int;

    public static var CONTROL = 0;
    public static var EXPERIMENTAL = 1;
    public static var UNUSED = 2;
    public static var UNUSED_CONTROL = 3;
    public static var UNUSED_EXPERIMENTAL = 4;
  }
}
