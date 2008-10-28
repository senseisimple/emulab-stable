package
{
  import flash.display.DisplayObjectContainer;

  class MenuSliceDetail extends MenuState
  {
    public function MenuSliceDetail(newSliceName : String,
                                    newSliceId : String,
                                    newCredential : Credential) : void
    {
      sliceName = newSliceName;
      sliceId = newSliceId;
      credential = newCredential;
    }

    override public function init(newParent : DisplayObjectContainer) : void
    {
      parent = newParent;
      clip = new SliceDetailClip();
      parent.addChild(clip);

      clip.sliceName.text = sliceName;
      nodes = new ActiveNodes(parent, clip.nodeList, clip.description);
      cm = new ComponentManager(clip.cmSelect, clip.nodeList, nodes);
      credential.setupSlivers(cm.getCmCount());
      console = new Console(parent, nodes, cm, clip.consoleButton,
                            clip.createSliversButton, clip.bootSliversButton,
                            clip.deleteSliversButton, credential);
      console.discoverResources();
    }

    override public function cleanup() : void
    {
      console.cleanup();
      cm.cleanup();
      nodes.cleanup();
      clip.parent.removeChild(clip);
    }

    var sliceName : String;
    var sliceId : String;
    var credential : Credential;
    var parent : DisplayObjectContainer;
    var clip : SliceDetailClip;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var console : Console;
  }
}
