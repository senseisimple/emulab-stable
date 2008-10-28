package
{
  import flash.display.DisplayObjectContainer;
  import flash.events.MouseEvent;
  import flash.events.ErrorEvent;
  import fl.controls.Button;
  import com.mattism.http.xmlrpc.MethodFault;


  class Console
  {
    public function Console(parent : DisplayObjectContainer,
                            newNodes : ActiveNodes, newCm : ComponentManager,
                            newConsoleButton : Button,
                            newCreateSliversButton : Button,
                            newBootSliversButton : Button,
                            newDeleteSliversButton : Button,
                            newCredential : Credential) : void
    {
      nodes = newNodes;
      cm = newCm;
      consoleButton = newConsoleButton;
      consoleButton.label = "Console";
      consoleButton.addEventListener(MouseEvent.CLICK, clickConsole);
      createSliversButton = newCreateSliversButton;
      createSliversButton.label = "Create Slivers";
      createSliversButton.addEventListener(MouseEvent.CLICK,
                                           clickCreateSlivers);
      bootSliversButton = newBootSliversButton;
      bootSliversButton.label = "Boot Slivers";
      bootSliversButton.addEventListener(MouseEvent.CLICK,
                                         clickBootSlivers);
      deleteSliversButton = newDeleteSliversButton;
      deleteSliversButton.label = "Delete Slivers";
      deleteSliversButton.addEventListener(MouseEvent.CLICK,
                                           clickDeleteSlivers);

      credential = newCredential;

      clip = new ConsoleClip();
      parent.addChild(clip);
      clip.back.label = "Back";
      clip.back.addEventListener(MouseEvent.CLICK, clickBack);
      clip.visible = false;

      queue = new DList();
      working = false;
    }

    public function cleanup() : void
    {
      consoleButton.removeEventListener(MouseEvent.CLICK, clickConsole);
      createSliversButton.removeEventListener(MouseEvent.CLICK,
                                              clickCreateSlivers);
      bootSliversButton.removeEventListener(MouseEvent.CLICK,
                                            clickBootSlivers);
      clip.back.removeEventListener(MouseEvent.CLICK, clickBack);

      clip.parent.removeChild(clip);
    }

    public function discoverResources() : void
    {
      forEachComponent(discoverSliver);
    }

    function discoverSliver(index : int) : Request
    {
      var result : Request = null;
      if (cm.getUrl(index) != null)
      {
        result = new RequestResourceDiscovery(index, cm);
      }
      return result;
    }

    function clickConsole(event : MouseEvent) : void
    {
      clip.visible = true;
    }

    function forEachComponent(func : Function) : void
    {
      var i : int = 0;
      for (; i < cm.getCmCount(); ++i)
      {
        if (cm.getUrl(i) != null)
        {
          var newRequest = func(i);
          if (newRequest != null)
          {
            queue.push_back(newRequest);
            nodes.changeState(i, ActiveNodes.PENDING);
            if (! working)
            {
              start();
            }
          }
        }
      }
    }

    function clickCreateSlivers(event : MouseEvent) : void
    {
      forEachComponent(createSliver);
    }

    function createSliver(index : int) : Request
    {
      var result : Request = null;
      var rspec = nodes.getXml(index);
      if (rspec != null)
      {
        if (credential.slivers[index] == null)
        {
          result = new RequestSliverCreate(index, nodes, cm, rspec);
        }
        else
        {
          result = new RequestSliverUpdate(index, nodes, cm, rspec);
        }
      }
      return result;
    }

    function clickBootSlivers(event : MouseEvent) : void
    {
      forEachComponent(bootSliver);
    }

    function bootSliver(index : int) : Request
    {
      var result : Request = null;
      if (nodes.existsState(index, ActiveNodes.CREATED))
      {
        result = new RequestSliverStart(index, nodes, cm.getUrl(index));
      }
      return result;
    }

    function clickDeleteSlivers(event : MouseEvent) : void
    {
      forEachComponent(deleteSliver);
    }

    function deleteSliver(index : int) : Request
    {
      var result : Request = null;
      if (nodes.existsState(index, ActiveNodes.PENDING)
          || nodes.existsState(index, ActiveNodes.CREATED)
          || nodes.existsState(index, ActiveNodes.BOOTED))
      {
        result = new RequestSliverDestroy(index, nodes, cm.getUrl(index));
      }
      return result;
    }

    function clickBack(event : MouseEvent) : void
    {
      clip.visible = false;
    }

    function start() : void
    {
      if (! queue.isEmpty())
      {
        var front = queue.front();
        var op = front.start(credential);
        clip.text.appendText(Util.getSend(front.getOpName(),
                                          ""));
        clip.text.scrollV = clip.text.maxScrollV;
        op.call(complete, failure);
        clip.text.appendText(front.getSendXml());
        clip.text.scrollV = clip.text.maxScrollV;
        working = true;
      }
      else
      {
        working = false;
      }
    }

    function failure(event : ErrorEvent, fault : MethodFault) : void
    {
      clip.text.appendText(Util.getFailure(queue.front().getOpName(),
                                           event, fault));
      clip.text.scrollV = clip.text.maxScrollV;
      queue.front().fail();
      queue.front().cleanup();
      queue.pop_front();
      start();
    }

    function complete(code : Number, response : Object) : void
    {
      clip.text.appendText(Util.getResponse(queue.front().getOpName(),
                                            queue.front().getResponseXml()));
      clip.text.scrollV = clip.text.maxScrollV;
      var next : Request = queue.front().complete(code, response, credential);
      queue.front().cleanup();
      queue.pop_front();
      if (next != null)
      {
        queue.push_back(next);
      }
      start();
    }

    var clip : ConsoleClip;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var consoleButton : Button;
    var createSliversButton : Button;
    var bootSliversButton : Button;
    var deleteSliversButton : Button;

    var queue : DList;
    var working : Boolean;

    var credential : Credential;
  }
}
