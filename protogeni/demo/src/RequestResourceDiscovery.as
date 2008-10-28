package
{
  class RequestResourceDiscovery extends Request
  {
    public function RequestResourceDiscovery(newCmIndex : int,
                                             newCm : ComponentManager) : void
    {
      cmIndex = newCmIndex;
      cm = newCm;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Requesting Resources";
      op.reset("cm", "DiscoverResources");
      op.addField("credential", credential.slice);
      op.setUrl(cm.getUrl(cmIndex));
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        cm.populateNodes(cmIndex, response.value);
      }
      else
      {
        cm.failResources(cmIndex);
      }
      return null;
    }

    var cmIndex : int;
    var cm : ComponentManager;
  }
}
