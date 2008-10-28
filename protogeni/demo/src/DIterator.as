package
{
  public class DIterator
  {
    public function DIterator() : void
    {
      this.current = null;
    }

    public function get() : *
    {
      if(this.current == null)
      {
        return null;
      }
      else
      {
        return this.current.data;
      }
    }

    public function isValid() : Boolean
    {
      return this.current != null;
    }

    public function increment() : void
    {
      if(this.current != null)
      {
        this.current = this.current.next;
      }
    }

    public function decrement() : void
    {
      if(this.current != null)
      {
        this.current = this.current.prev;
      }
    }

    public function copyFrom(right : DIterator) : void
    {
      this.current = right.current;
    }

    public function getNode() : DNode
    {
      return this.current;
    }

    public function setNode(newNode : DNode) : void
    {
      this.current = newNode;
    }

    protected var current : DNode;
  }
}
