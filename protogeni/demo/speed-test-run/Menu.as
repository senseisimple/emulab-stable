package
{
  import flash.events.Event;
  import flash.events.MouseEvent;
  import flash.events.ErrorEvent;
  import flash.text.TextField;
  import flash.net.navigateToURL;
  import flash.net.URLRequest;
  import com.mattism.http.xmlrpc.MethodFault;

  public class Menu
  {
    public function Menu(newChoice : MenuClip,
                         newProgress : WaitingClip) : void
    {
      choice = newChoice;
      progress = newProgress;
      progress.visible = false;
      xml = progress.xmlText;
      loadText = progress.loadText;

      buttons = new ButtonList([choice.okButton], [clickOk]);
      op = new Operation(null);

      reset();
    }

    public function getConsole() : TextField
    {
      return progress.xmlText;
    }

    private function clickOk(event : MouseEvent) : void
    {
      progress.addEventListener(Event.ENTER_FRAME, enterFrame);
      progress.visible = true;
      startCredential();
    }

    private function enterFrame(event : Event) : void
    {
      if (xml.scrollV < xml.maxScrollV)
      {
        xml.scrollV += 3;
      }
      progress.waitIcon.rotation += 5;
    }

    function failure(event : ErrorEvent, fault : MethodFault) : void
    {
      progress.removeEventListener(Event.ENTER_FRAME, enterFrame);
      if (fault != null)
      {
        xml.appendText("\nFAILURE fault: " + opName + ": "
                       + fault.getFaultString());
      }
      else
      {
        xml.appendText("\nFAILURE event: " + opName + ": "
                       + event.toString());
      }
      xml.appendText("\nURL: " + op.getUrl());
    }

    function codeFailure() : void
    {
      progress.removeEventListener(Event.ENTER_FRAME, enterFrame);
      progress.loadText.text = "Code Failure: " + opName;
    }

    function addSend() : void
    {
      var before : int = xml.scrollV;
      xml.appendText("\n-----------------------------------------\n");
      xml.appendText("SEND: " + opName + "\n");
      xml.appendText("URL: " + op.getUrl() + "\n");
      xml.appendText("-----------------------------------------\n\n");
      xml.appendText(op.getSendXml());
      xml.scrollV = before;
//      clip.xmlText.scrollV = clip.xmlText.maxScrollV;
    }

    function addResponse() : void
    {
      var before : int = xml.scrollV;
      xml.appendText("\n-----------------------------------------\n");
      xml.appendText("RESPONSE: " + opName + "\n");
      xml.appendText("-----------------------------------------\n\n");
      xml.appendText(op.getResponseXml());
      xml.scrollV = before;
    }

    function startCredential() : void
    {
      opName = "Getting User credential";
      loadText.text = opName;
      op.reset(Geni.getCredential);
      op.call(completeCredential, failure);
      addSend();
    }

    function completeCredential(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        user = String(response.value);
        startSliceName();
      }
      else
      {
        codeFailure();
      }
    }

    function startSliceName() : void
    {
      opName = "Getting Slice Name";
      loadText.text = opName;
      op.reset(Geni.resolve);
      op.addField("credential", user);
      op.addField("hrn", choice.sliceName.text);
      op.addField("type", "Slice");
      op.call(completeSliceName, failure);
      addSend();
    }

    function completeSliceName(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        sliceName = response.value.uuid;
        sliceUrn = response.value.urn;
        approvedManagers = response.value.component_managers;
        startSlice();
      }
      else
      {
        codeFailure();
      }
    }

    function startSlice() : void
    {
      opName = "Getting Slice Credential";
      loadText.text = opName;
      op.reset(Geni.getCredential);
      op.addField("credential", user);
      op.addField("uuid", sliceName);
      op.addField("type", "Slice");
      op.call(completeSlice, failure);
      addSend();
    }

    function completeSlice(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        slice = String(response.value);
        startListComponents();
      }
      else
      {
        codeFailure();
      }
    }

    public function startListComponents() : void
    {
      opName = "Looking up components";
      loadText.text = opName;
      op.reset(Geni.listComponents);
      op.addField("credential", user);
      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
      op.call(completeListComponents, failure);
      addSend();
    }

    public function completeListComponents(code : Number,
                                           response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        for each(var obj:Object in response.value)
        {
          managers.push(obj.url);
          managerUrns.push(obj.urn);
        }
        startSliverName();
      }
      else
      {
        codeFailure();
      }
    }

    public function startSliverName() : void
    {
      var done : Boolean = false;
      while (managers.length > 0 && !done)
      {
        if (approvedManagers.indexOf(managerUrns[managerUrns.length - 1]) == -1)
        {
          managers.pop();
          managerUrns.pop();
        }
        else
        {
          done = true;
        }
      }
      if (managers.length > 0)
      {
        opName = "Looking up sliver name";
        loadText.text = opName;
        op.reset(Geni.resolve);
        op.addField("credentials", new Array(slice));
        op.addField("urn", sliceUrn);
        op.setExactUrl(managers[managers.length - 1]);
        op.call(completeSliverName, failSliverName);
        addSend();
      }
      else
      {
        openPage();
      }
    }

    function failSliverName(event : ErrorEvent, fault : MethodFault) : void
    {
      managers.pop();
      managerUrns.pop();
      startSliverName();
    }

    public function completeSliverName(code : Number,
                                       response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        sliverName = String(response.value.sliver_urn);
        startManifest();
      }
      else
      {
        managers.pop();
        managerUrns.pop();
        startSliverName();
      }
    }

    public function startManifest() : void
    {
      opName = "Looking up sliver manifest";
      loadText.text = opName;
      op.reset(Geni.resolve);
      op.addField("credentials", new Array(slice))
      op.addField("urn", sliverName);
      op.setExactUrl(managers[managers.length - 1]);
      op.call(completeManifest, failSliverName);
      addSend();
    }

    public function completeManifest(code : Number,
                                     response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        addHosts(String(response.value.manifest));
        managers.pop();
        managerUrns.pop();
        startSliverName();
      }
      else
      {
        managers.pop();
        managerUrns.pop();
        startSliverName();
      }
    }

    public function addHosts(manifestString : String) : void
    {
      var manifest : XML = XML(manifestString);
      var nodeName : QName = new QName(manifest.namespace(), "node");
      for each (var element in manifest.elements(nodeName))
      {
        var newHost = element.attribute("hostname");
        if (newHost != null && newHost.length() > 0)
        {
          hosts.push(String(newHost));
        }
      }
    }

    public function openPage() : void
    {
      progress.removeEventListener(Event.ENTER_FRAME, enterFrame);
      if (hosts.length > 0)
      {
//        progress.visible = false;
        var url = "http://boss.emulab.net/all-speed-test.php?";
        url += "names=" + hosts.join(",") + "&";
        url += "cms=" + hosts.join(",") + "&";
        url += "hostnames=" + hosts.join(",");
        navigateToURL(new URLRequest(url), "_blank");
      }
      else
      {
        xml.appendText("\n\n\n\nHost List Empty\n");
      }
      xml.scrollV = xml.maxScrollV;
      reset();
    }

    private function reset() : void
    {
      sliceName = null;
      sliceUrn = null;
      sliverName = null;
      user = null;
      slice = null;
      managers = new Array();
      managerUrns = new Array();
      hosts = new Array();
    }

    private var choice : MenuClip;
    private var progress : WaitingClip;
    private var buttons : ButtonList;
    private var xml : TextField;
    private var loadText : TextField;

    private var op : Operation;
    private var opName : String;
    private var sliceName : String;
    private var sliceUrn : String;
    private var approvedManagers : Array;
    private var sliverName : String;
    private var user : String;
    private var slice : String;
    private var managers : Array;
    private var managerUrns : Array;

    private var hosts : Array;
  }
}
