/* EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
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
  class Credential
  {
    public function Credential() : void
    {
      base = null;
      slice = null;
      slivers = null;
      ssh = null;
    }

    public function setupSlivers(count : int)
    {
      slivers = new Array();
      var i : int = 0;
      for (; i < count; ++i)
      {
        slivers.push(null);
      }
    }

    public var base : String;
    public var slice : String;
    public var slivers : Array;
    public var ssh : Object;
  }
}
