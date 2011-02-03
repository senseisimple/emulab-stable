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
	import flash.utils.Dictionary;

  public class RequestQueue
  {
    public function RequestQueue(shouldPushEvents:Boolean = false) : void
    {
      head = null;
	  nextRequest = null;
      tail = null;
	  pushEvents = shouldPushEvents;
    }

    public function isEmpty() : Boolean
    {
      return head == null;
    }
	
	public function contains(item:*):Boolean
	{
		var parseNode:RequestQueueNode = head;
		if(item is RequestQueueNode)
		{
			while(parseNode != null)
			{
				if(parseNode == item)
					return true;
				parseNode = parseNode.next;
			}
		} else {
			while(parseNode != null)
			{
				if(parseNode.item == item)
					return true;
				parseNode = parseNode.next;
			}
		}
		return false;
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
		{
			if(newNode.item.forceNext && nextRequest != null)
			{
				var parseNode:RequestQueueNode = head;
				if(parseNode == nextRequest) {
					head = newNode;
				} else {
					while(parseNode.next != nextRequest)
						parseNode = parseNode.next;
					parseNode.next = newNode;
				}
				
				newTail.next = nextRequest;
				nextRequest = newNode;
				newTail = tail;
			} else {
				tail.next = newNode;
				if(nextRequest == null)
					nextRequest = newNode;
			}
			
		}
		else
		{
			head = newNode;
			nextRequest = newNode;
		}
		tail = newTail;
		
		if(pushEvents)
			Main.geniHandler.dispatchQueueChanged();
    }
	
	public function working():Boolean
	{
		return head != null && nextRequest != head;
	}
	
	public function workingCount():int
	{
		var count:int = 0;
		var n:RequestQueueNode = head;
		
		while(n != null && n != nextRequest)
		{
			if((n.item as Request).running)
				count++;
			n = n.next;
		}

		return count;
	}
	
	public function readyToStart():Boolean
	{
		return head != null && nextRequest != null && (nextRequest == head || nextRequest.item.startImmediately == true);
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
	
	public function nextAndProgress() : *
	{
		var val:Object = next();
		if(val != null)
			nextRequest = nextRequest.next;
		return val;
	}
	
	public function next() : *
	{
		if (nextRequest != null)
		{
			return nextRequest.item;
		}
		else
		{
			return null;
		}
	}

	/*
    public function pop() : void
    {
      if (head != null)
      {
		if(nextRequest == head)
			nextRequest = head.next;
        head = head.next;
		if(pushEvents)
			Main.geniHandler.dispatchQueueChanged();
      }
      if (head == null)
      {
        tail = null;
		nextRequest = null;
      }
    }
	*/
	
	public function getRequestQueueNodeFor(item:Request):RequestQueueNode
	{
		var parseNode:RequestQueueNode = head;
		while(parseNode != null)
		{
			if(parseNode.item == item)
				return parseNode;
			parseNode = parseNode.next;
		}
		return null;
	}
	
	public function remove(removeNode:RequestQueueNode):void
	{
		if(removeNode == null)
			return;
		if(head == removeNode)
		{
			if(nextRequest == head)
				nextRequest = head.next;
			head = head.next;
			if(head == null)
				tail = null;
		} else {
			var previousNode:RequestQueueNode = head;
			var currentNode:RequestQueueNode = head.next;
			while(currentNode != null)
			{
				if(currentNode == removeNode)
				{
					if(nextRequest == currentNode)
						nextRequest = currentNode.next;
					previousNode.next = currentNode.next;
					return;
				}
				previousNode = previousNode.next;
				currentNode = currentNode.next;
			}
		}
		if(pushEvents)
			Main.geniHandler.dispatchQueueChanged();
	}

    public var head:RequestQueueNode;
    public var tail:RequestQueueNode;
	public var nextRequest:RequestQueueNode;
	private var pushEvents:Boolean;
  }
}
