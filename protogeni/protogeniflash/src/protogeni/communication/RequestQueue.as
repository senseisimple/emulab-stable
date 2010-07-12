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

package protogeni.communication
{
  public class RequestQueue
  {
    public function RequestQueue() : void
    {
      head = null;
      tail = null;
    }

    public function isEmpty() : Boolean
    {
      return head == null;
    }

    public function push(newItem:*) : void
    {
		var newNode:RequestQueueNode = null;
		var newTail:RequestQueueNode = null;
		
		if(newItem is RequestQueueNode)
		{
			newNode = newItem;
			newTail = newNode;
			while(newTail.next != null)
				newTail = newTail.next;
		}
		else
		{
			newNode = new RequestQueueNode(newItem);
			newTail = newNode;
		}
		
		if (tail != null)
			tail.next = newNode;
		else
			head = newNode;
		
		tail = newTail;
		Main.protogeniHandler.dispatchQueueChanged();
    }

    public function front() : *
    {
      if (head != null)
      {
        return head.item;
      }
      else
      {
        return null;
      }
    }

    public function pop() : void
    {
      if (head != null)
      {
        head = head.next;
		Main.protogeniHandler.dispatchQueueChanged();
      }
      if (head == null)
      {
        tail = null;
      }
    }

    public var head : RequestQueueNode;
    public var tail : RequestQueueNode;
  }
}
