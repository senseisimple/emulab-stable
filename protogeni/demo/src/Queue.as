package
{
  class Queue
  {
    public function Queue() : void
    {
      head = null;
      tail = null;
    }

    public function isEmpty() : Boolean
    {
      return head == null;
    }

    public function push(newItem : *) : void
    {
      var newNode = new QueueNode(newItem);
      if (tail != null)
      {
        tail.next = newNode;
      }
      else
      {
        head = newNode;
      }
      tail = newNode;
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
      }
      if (head == null)
      {
        tail = null;
      }
    }

    var head : QueueNode;
    var tail : QueueNode;
  }
}
