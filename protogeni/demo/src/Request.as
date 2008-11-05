package
{
  class Request
  {
    public function Request() : void
    {
      op = new Operation(null);
      opName = "";
    }

    public function cleanup() : void
    {
      op.cleanup();
    }

    public function getSendXml() : String
    {
      return op.getSendXml();
    }

    public function getResponseXml() : String
    {
      return op.getResponseXml();
    }

    public function getOpName() : String
    {
      return opName;
    }

    public function start(credential : Credential) : Operation
    {
      return op;
    }

    public function complete(code : Number, response : Object,
                             credential : Credential) : Request
    {
      return null;
    }

    public function fail() : void
    {
    }

    protected var op : Operation;
    protected var opName : String;

    public static var IMPOTENT : int = 0;
  }
}
