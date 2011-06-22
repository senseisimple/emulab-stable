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
	 * Federated ProtoGENI manager
	 * 
	 * @author mstrum
	 * 
	 */
	public class ProtogeniComponentManager extends GeniManager
	{
		public static const LEVEL_MINIMAL:int = 0;
		public static const LEVEL_FULL:int = 1;
		
		public var Level:int;
		
		public function ProtogeniComponentManager()
		{
			super();
			this.rspecProcessor = new ProtogeniRspecProcessor(this);
			this.type = GeniManager.TYPE_PROTOGENI;
		}
	}
}