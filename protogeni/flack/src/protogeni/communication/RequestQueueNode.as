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

package protogeni.communication
{
	/**
	 * Holds a request for a queue
	 * 
	 * @author mstrum
	 * 
	 */
	public class RequestQueueNode
	{
		public var _item:*;
		public function get item():* { return _item; }
		public function set item(newItem:*):void {
			_item = newItem;
			if(_item != null)
				_item.node = this;
		}
		public var next:RequestQueueNode;
		
		public function RequestQueueNode(newItem:* = null):void
		{
			item = newItem;
			next = null;
		}
	}
}
