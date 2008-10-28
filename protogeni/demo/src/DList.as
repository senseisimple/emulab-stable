package
{
  public class DList
  {
    public function DList() : void
    {
      this.head = null;
      this.tail = null;
      currentSize = 0;
    }

    public function size() : int
    {
      return currentSize;
    }

    public function isEmpty() : Boolean
    {
      return this.head == null;
    }

    public function frontIterator() : DIterator
    {
      var result : DIterator = new DIterator();
      result.setNode(this.head);
      return result;
    }

    public function backIterator() : DIterator
    {
      var result : DIterator = new DIterator();
      result.setNode(this.tail);
      return result;
    }

    public function insert(pos : DIterator,newData : *) : DIterator
    {
      var result : DIterator = new DIterator();
      if(!pos.isValid())
      {
        this.push_back(newData);
        result.setNode(this.tail);
      }
      else if(pos.getNode() == this.head)
      {
        this.push_front(newData);
        result.setNode(this.head);
      }
      else
      {
        var newNode : DNode = new DNode();
        newNode.data = newData;
        newNode.prev = pos.getNode().prev;
        newNode.next = pos.getNode();
        pos.getNode().prev.next = newNode;
        pos.getNode().prev = newNode;
        result.setNode(newNode);
      }
      ++currentSize;
      return result;
    }

    public function erase(pos : DIterator) : DIterator
    {
      var result : DIterator = new DIterator();
      if(pos.isValid())
      {
        if(pos.getNode() == this.head)
        {
          this.pop_front();
          result.setNode(this.head);
        }
        else if(pos.getNode() == this.tail)
        {
          this.pop_back();
          result.setNode(this.tail);
        }
        else
        {
          pos.getNode().prev.next = pos.getNode().next;
          pos.getNode().next.prev = pos.getNode().prev;
          result.setNode(pos.getNode().next);
        }
        --currentSize;
      }
      return result;
    }

    public function remove(value : *) : void
    {
      var pos = find(value);
      if (pos.isValid())
      {
        erase(pos);
      }
    }

    // Returns an iterator pointing to value or an invalid iterator
    public function find(value : *) : DIterator
    {
      var current = frontIterator();
      for (; current.isValid(); current.increment())
      {
        if (current.get() == value)
        {
          break;
        }
      }
      return current;
    }

    public function front() : *
    {
      if(this.head != null)
      {
        return this.head.data;
      }
      else
      {
        return null;
      }
    }

    public function back() : *
    {
      if(this.tail != null)
      {
        return this.tail.data;
      }
      else
      {
        return null;
      }
    }

    public function push_front(newData : *) : void
    {
      var newNode : DNode = new DNode();
      newNode.data = newData;
      if(this.head == null)
      {
        this.head = newNode;
        this.tail = newNode;
      }
      else
      {
        this.head.prev = newNode;
        newNode.next = this.head;
        this.head = newNode;
      }
      ++currentSize;
    }

    public function push_back(newData : *) : void
    {
      var newNode : DNode = new DNode();
      newNode.data = newData;
      if(this.tail == null)
      {
        this.head = newNode;
        this.tail = newNode;
      }
      else
      {
        this.tail.next = newNode;
        newNode.prev = this.tail;
        this.tail = newNode;
      }
      ++currentSize;
    }

    public function pop_front() : void
    {
      if (this.head != null && this.tail != null)
      {
        if(this.head == this.tail)
        {
          this.head = null;
          this.tail = null;
        }
        else
        {
          this.head.next.prev = null;
          this.head = this.head.next;
        }
        --currentSize;
      }
    }

    public function pop_back() : void
    {
      if (this.head != null && this.tail != null)
      {
        if(this.head == this.tail)
        {
          this.head = null;
          this.tail = null;
        }
        else
        {
          this.tail.prev.next = null;
          this.tail = this.tail.prev;
        }
        --currentSize;
      }
    }

    protected var head : DNode;
    protected var tail : DNode;
    protected var currentSize : int;
  }
}
