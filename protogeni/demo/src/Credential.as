package
{
  class Credential
  {
    public function Credential() : void
    {
      base = null;
      slice = null;
      slivers = null;
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
  }
}
