/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
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

package protogeni.resources
{
	/**
	 * Describes the sliver environment which will be given to the user
	 * 
	 * @author mstrum
	 * 
	 */
	public class SliverType
	{
		public var name:String;
		public var diskImages:Vector.<DiskImage>;
		
		public function SliverType(newName:String = "")
		{
			this.name = newName;
			this.diskImages = new Vector.<DiskImage>();
		}
	}
}