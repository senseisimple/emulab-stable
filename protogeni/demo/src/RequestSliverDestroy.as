package
{
  class RequestSliverDestroy extends Request
  {
    public function RequestSliverDestroy(newCmIndex : int,
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
      // TODO: Check to make sure that credential.slivers[cmIndex]
      // exists and perform a no-op if it doesn't.
      opName = "Deleting Sliver";
      op.reset(Geni.deleteSliver);
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
        nodes.changeState(cmIndex, ActiveNodes.PLANNED);
      }
      else
      {
        nodes.changeState(cmIndex, ActiveNodes.FAILED);
      }
      credential.slivers[cmIndex] = null;
      return null;
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var url : String;
  }
}
