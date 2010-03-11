/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */

package
{
  import flash.display.DisplayObjectContainer;
  import flash.display.StageDisplayState;
  import flash.events.MouseEvent;
  import flash.events.ErrorEvent;
  import flash.net.navigateToURL;
  import flash.net.URLRequest;
  import flash.system.System;
  import flash.text.TextField;

  import fl.controls.Button;

  import com.mattism.http.xmlrpc.MethodFault;


  class Console
  {
    public function Console(parent : DisplayObjectContainer,
                            newNodes : ActiveNodes,
                            newManagers : ComponentView,
                            newArena : SliceDetailClip,
                            newCredential : Credential,
                            newText : String,
                            newSliceName : String) : void
    {
      nodes = newNodes;
      managers = newManagers;
      arena = newArena;
      credential = newCredential;
      sliceUrn = Util.makeUrn(Geni.defaultAuthority, "slice", newSliceName);

      clip = new ConsoleClip();
      parent.addChild(clip);
      clip.visible = false;
      clip.text.text = newText;

      clip.rspecText.visible = false;
      clip.rspecScroll.visible = false;

      var buttons = new ButtonList(new Array(arena.consoleButton,
                                             arena.createButton,
                                             arena.bootButton,
                                             arena.speedButton,
                                             arena.deleteButton,
                                             arena.embedButton,
                                             arena.rspecButton,
                                             arena.fullScreenButton,
                                             clip.backButton,
                                             clip.copyButton),
                                   new Array(clickConsole,
                                             clickCreateSlivers,
                                             clickBootSlivers,
                                             clickRunSpeedTest,
                                             clickDeleteSlivers,
                                             clickEmbed,
                                             clickRspec,
                                             clickFullScreen,
                                             clickBack,
                                             clickCopy));

      queue = new Queue();
      working = false;
    }

    public function cleanup() : void
    {
      buttons.cleanup();
      clip.parent.removeChild(clip);
    }

    public function discoverResources() : void
    {
      pushRequest(new RequestListComponents(managers));
      forEachComponent(discoverSliver);
    }

    public function getConsole() : TextField
    {
      return clip.text;
    }

    function discoverSliver(cm : ComponentManager) : Request
    {
      if (cm != managers.getManagers()[0])
      {
        if (cm.getName() == "Georgia Tech")
        {
          cm.resourceSuccess(ComponentView.gaAd);
          return null;
        }
        else
        {
          return new RequestResourceDiscovery(cm, null);
        }
      }
      else
      {
        return null;
      }
    }

    function clickConsole(event : MouseEvent) : void
    {
      clip.text.visible = true;
      clip.textScroll.visible = true;
      clip.rspecText.visible = false;
      clip.rspecScroll.visible = false;
      clip.visible = true;
    }

    function forEachComponent(func : Function) : void
    {
      for each (var cm in managers.getManagers())
      {
        if (cm.getUrl() != null)
        {
          var newRequest = func(cm);
          pushRequest(newRequest);
        }
      }
    }

    function pushRequest(newRequest : Request) : void
    {
      if (newRequest != null)
      {
        queue.push(newRequest);
        if (! working)
        {
          start();
        }
      }
    }

    function clickCreateSlivers(event : MouseEvent) : void
    {
      if (queue.isEmpty())
      {
        forEachComponent(createSliver);
      }
    }

    function createSliver(cm : ComponentManager) : Request
    {
      var result : Request = null;
      var shouldSend = nodes.managerUsed(cm);
      if (shouldSend && cm.hasChanged() && cm.getName() != "Georgia Tech")
      {
        cm.clearChanged();
        var rspec = null;
        if (cm.getSliver() == null)
        {
          rspec = nodes.getXml(false, cm);
          result = new RequestSliverCreate(cm, nodes, rspec, sliceUrn);
        }
        else if (cm.getVersion() >= 2)
        {
          rspec = nodes.getXml(true, cm);
          result = new RequestSliverUpdate(cm, nodes, rspec, false, sliceUrn);
        }
      }
      return result;
    }

    function clickBootSlivers(event : MouseEvent) : void
    {
      if (queue.isEmpty())
      {
        forEachComponent(bootSliver);
      }
    }

    function bootSliver(cm : ComponentManager) : Request
    {
      var result : Request = null;
      if (nodes.existsState(cm, ActiveNodes.CREATED))
      {
        result = new RequestSliverStart(cm, nodes, sliceUrn);
      }
      return result;
    }

    function clickRunSpeedTest(event : MouseEvent) : void
    {
      if (queue.isEmpty())
      {
        var urlText = nodes.getSpeedUrl();
        if (urlText != null)
        {
          var url : URLRequest = new URLRequest(urlText);
          navigateToURL(url, "_blank");
        }
      }
    }

    function clickDeleteSlivers(event : MouseEvent) : void
    {
      if (queue.isEmpty())
      {
        forEachComponent(deleteSliver);
      }
    }

    function deleteSliver(cm : ComponentManager) : Request
    {
      var result : Request = null;
      if (nodes.existsState(cm, ActiveNodes.PENDING)
          || nodes.existsState(cm, ActiveNodes.CREATED)
          || nodes.existsState(cm, ActiveNodes.BOOTED))
      {
        result = new RequestSliverDestroy(cm, nodes, sliceUrn);
      }
      return result;
    }

    function clickEmbed(event : MouseEvent) : void
    {
      if (managers.isValidManager() && queue.isEmpty())
      {
        var currentManager = managers.getCurrentManager();
        var request = new RequestSliceEmbedding(currentManager, nodes,
                                                nodes.getXml(false,
                                                             currentManager),
                                                Geni.sesUrl);
        pushRequest(request);
      }
    }

    function clickFullScreen(event : MouseEvent) : void
    {
      if (arena.stage.displayState == StageDisplayState.FULL_SCREEN)
      {
        arena.stage.displayState = StageDisplayState.NORMAL;
      }
      else
      {
        arena.stage.displayState = StageDisplayState.FULL_SCREEN;
      }
    }

    function clickBack(event : MouseEvent) : void
    {
      clip.visible = false;
    }

    function clickCopy(event : MouseEvent) : void
    {
      if (clip.text.visible)
      {
        System.setClipboard(clip.text.text);
      }
      else if (clip.rspecText.visible)
      {
        System.setClipboard(clip.rspecText.text);
      }
    }

    function clickRspec(event : MouseEvent) : void
    {
      clip.text.visible = false;
      clip.textScroll.visible = false;
      clip.rspecText.visible = true;
      clip.rspecScroll.visible = true;
      clip.visible = true;

      var managerName = managers.getCurrentManager().getName();
      clip.rspecText.text = "";
      clip.rspecText.appendText("--------------------------------------\n");
      clip.rspecText.appendText("Request: " + managerName + "\n");
      clip.rspecText.appendText("--------------------------------------\n\n");
      var xml = nodes.getXml(true, managers.getCurrentManager());
      clip.rspecText.appendText(xml.toString());

      clip.rspecText.appendText("\n\n--------------------------------------\n");
      clip.rspecText.appendText("Advertisement: " + managerName + "\n");
      clip.rspecText.appendText("--------------------------------------\n\n");
      clip.rspecText.appendText(managers.getCurrentManager().getAd());

      clip.rspecText.appendText("\n\n--------------------------------------\n");
      clip.rspecText.appendText("Manifest: " + managerName + "\n");
      clip.rspecText.appendText("--------------------------------------\n\n");
      clip.rspecText.appendText(managers.getCurrentManager().getManifest());

      clip.rspecScroll.update();
    }

    function start() : void
    {
      if (! queue.isEmpty())
      {
        var front = queue.front();
        var op = front.start(credential);
        arena.opText.text = front.getOpName();
        clip.text.appendText(Util.getSend(front.getOpName(), front.getUrl(),
                                          ""));
        clip.text.scrollV = clip.text.maxScrollV;
        op.call(complete, failure);
        clip.text.appendText(front.getSendXml());
        clip.text.scrollV = clip.text.maxScrollV;
        working = true;
        changeButtonState(ButtonList.GHOSTED);
      }
      else
      {
        working = false;
        changeButtonState(ButtonList.NORMAL);
      }
    }

    function changeButtonState(newState : int) : void
    {
      arena.createButton.gotoAndStop(newState);
      arena.bootButton.gotoAndStop(newState);
      arena.speedButton.gotoAndStop(newState);
      arena.deleteButton.gotoAndStop(newState);
      arena.embedButton.gotoAndStop(newState);
    }

    function failure(event : ErrorEvent, fault : MethodFault) : void
    {
      clip.text.appendText(Util.getFailure(queue.front().getOpName(),
                                           queue.front().getUrl(),
                                           event, fault));
      clip.text.scrollV = clip.text.maxScrollV;
      var next : Request = queue.front().fail();
      queue.front().cleanup();
      queue.pop();
      if (next != null)
      {
        queue.push(next);
      }
      start();
    }

    function complete(code : Number, response : Object) : void
    {
      try
      {
        clip.text.appendText(Util.getResponse(queue.front().getOpName(),
                                              queue.front().getUrl(),
                                              queue.front().getResponseXml()));
        clip.text.scrollV = clip.text.maxScrollV;
        var next : Request = queue.front().complete(code, response, credential);
        queue.front().cleanup();
        queue.pop();
        if (next != null)
        {
          queue.push(next);
        }
        start();
      }
      catch (e : Error)
      {
        clip.text.appendText("\n" + e.toString() + "\n\n" + e.getStackTrace());
      }
    }

    public function isWorking() : Boolean
    {
      return working;
    }

    var clip : ConsoleClip;
    var nodes : ActiveNodes;
    var managers : ComponentView;
    var arena : SliceDetailClip;

    var buttons : ButtonList;

    var queue : Queue;
    var working : Boolean;

    var credential : Credential;
    var sliceUrn : String;
  }
}
