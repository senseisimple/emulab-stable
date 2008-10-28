package
{
  class RequestSliverStart extends Request
  {
    public function RequestSliverStart(newCmIndex : int,
                                       newNodes : ActiveNodes,
                                       newUrl : String) : void
    {
      cmIndex = newCmIndex;
      nodes = newNodes;
      url = newUrl;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Booting Sliver";
      op.reset("cm", "StartSliver");
      op.addField("credential", credential.slivers[cmIndex]);
      op.addField("impotent", Request.IMPOTENT);
      op.setUrl(url);
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        nodes.changeState(cmIndex, ActiveNodes.BOOTED);
      }
      else
      {
        nodes.changeState(cmIndex, ActiveNodes.FAILED);
      }
      return null;
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var url : String;
  }
}
