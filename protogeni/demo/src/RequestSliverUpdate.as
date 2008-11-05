package
{
  class RequestSliverUpdate extends Request
  {
    public function RequestSliverUpdate(newCmIndex : int,
                                        newNodes : ActiveNodes,
                                        newCm : ComponentManager,
                                        newRspec : String) : void
    {
      cmIndex = newCmIndex;
      nodes = newNodes;
      cm = newCm;
      rspec = newRspec;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Updating Sliver";
      op.reset(Geni.updateSliver);
      op.addField("credential", credential.slivers[cmIndex]);
      op.addField("rspec", rspec);
      op.addField("impotent", Request.IMPOTENT);
      op.setUrl(cm.getUrl(cmIndex));
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        nodes.changeState(cmIndex, ActiveNodes.CREATED);
      }
      else
      {
        nodes.changeState(cmIndex, ActiveNodes.FAILED);
      }
      return null;
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var rspec : String;
  }
}
