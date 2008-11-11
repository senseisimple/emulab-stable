package
{
  class QueueNode
  {
    public function QueueNode(newItem : *) : void
    {
      item = newItem;
      next = null;
    }

    public var item : *;
    public var next : QueueNode;
  }
}
