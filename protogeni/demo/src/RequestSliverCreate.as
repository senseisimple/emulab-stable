package
{
  class RequestSliverCreate extends Request
  {
    public function RequestSliverCreate(newCmIndex : int,
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
      if (cm.getTicket(cmIndex) == null)
      {
        opName = "Getting Ticket";
        op.reset(Geni.getTicket);
        op.addField("credential", credential.slice);
        op.addField("rspec", rspec);
        op.addField("impotent", Request.IMPOTENT);
        op.setUrl(cm.getUrl(cmIndex));
      }
      else
      {
        opName = "Redeeming Ticket";
        op.reset(Geni.redeemTicket);
        op.addField("ticket", cm.getTicket(cmIndex));
        op.addField("impotent", Request.IMPOTENT);
        op.setUrl(cm.getUrl(cmIndex));
      }
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = null;
      if (code == 0)
      {
        if (cm.getTicket(cmIndex) == null)
        {
          cm.setTicket(cmIndex, response.value);
          result = new RequestSliverCreate(cmIndex, nodes, cm, rspec);
        }
        else
        {
          nodes.changeState(cmIndex, ActiveNodes.CREATED);
          credential.slivers[cmIndex] = response.value;
          cm.setTicket(cmIndex, null);
        }
      }
      else
      {
        nodes.changeState(cmIndex, ActiveNodes.FAILED);
      }
      return result;
    }

    override public function fail() : void
    {
      nodes.changeState(cmIndex, ActiveNodes.FAILED);
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var rspec : String;
  }
}
