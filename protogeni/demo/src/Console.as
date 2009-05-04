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
  import flash.text.TextField;
  import flash.events.MouseEvent;
  import flash.events.ErrorEvent;
  import flash.net.navigateToURL;
  import flash.net.URLRequest;
  import fl.controls.Button;
  import com.mattism.http.xmlrpc.MethodFault;


  class Console
  {
    public function Console(parent : DisplayObjectContainer,
                            newNodes : ActiveNodes, newCm : ComponentManager,
                            newArena : SliceDetailClip,
                            newCredential : Credential,
                            newText : String) : void
    {
      nodes = newNodes;
      cm = newCm;
      arena = newArena;
/*
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
      runSpeedTestButton = newRunSpeedTestButton;
      runSpeedTestButton.label = "Speed Test";
      runSpeedTestButton.addEventListener(MouseEvent.CLICK,
                                          clickRunSpeedTest);
      deleteSliversButton = newDeleteSliversButton;
      deleteSliversButton.label = "Delete Slivers";
      deleteSliversButton.addEventListener(MouseEvent.CLICK,
                                           clickDeleteSlivers);
*/
      credential = newCredential;

      clip = new ConsoleClip();
      parent.addChild(clip);
/*
      clip.back.label = "Back";
      clip.back.addEventListener(MouseEvent.CLICK, clickBack);
      clip.rspec.label = "RSpec";
      clip.rspec.addEventListener(MouseEvent.CLICK, clickRspec);
*/
      clip.visible = false;
      clip.text.text = newText;

      clip.rspecText.visible = false;
      clip.rspecScroll.visible = false;

      var buttons = new ButtonList(new Array(arena.consoleButton,
                                             arena.createButton,
                                             arena.bootButton,
                                             arena.speedButton,
                                             arena.deleteButton,
                                             clip.backButton,
                                             clip.rspecButton),
                                   new Array(clickConsole,
                                             clickCreateSlivers,
                                             clickBootSlivers,
                                             clickRunSpeedTest,
                                             clickDeleteSlivers,
                                             clickBack,
                                             clickRspec));

      queue = new Queue();
      working = false;
    }

    public function cleanup() : void
    {
/*
      consoleButton.removeEventListener(MouseEvent.CLICK, clickConsole);
      createSliversButton.removeEventListener(MouseEvent.CLICK,
                                              clickCreateSlivers);
      bootSliversButton.removeEventListener(MouseEvent.CLICK,
                                            clickBootSlivers);
      runSpeedTestButton.removeEventListener(MouseEvent.CLICK,
                                          clickRunSpeedTest);
      clip.back.removeEventListener(MouseEvent.CLICK, clickBack);
      clip.rspec.removeEventListener(MouseEvent.CLICK, clickRspec);
*/
      buttons.cleanup();
      clip.parent.removeChild(clip);
    }

    public function discoverResources() : void
    {
      forEachComponent(discoverSliver);
    }

    public function getConsole() : TextField
    {
      return clip.text;
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
            queue.push(newRequest);
//            nodes.changeState(i, ActiveNodes.PENDING);
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
      if (queue.isEmpty())
      {
        forEachComponent(createSliver);
      }
    }

    function createSliver(index : int) : Request
    {
      var result : Request = null;
      var rspec = nodes.getXml(index, false);
      if (rspec != null)
      {
        if (credential.slivers[index] == null)
        {
          result = new RequestSliverCreate(index, nodes, cm, rspec);
        }
        else
        {
          rspec = nodes.getXml(index, true);
          result = new RequestSliverUpdate(index, nodes, cm, rspec, true);
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

    function bootSliver(index : int) : Request
    {
      var result : Request = null;
      if (nodes.existsState(index, ActiveNodes.CREATED))
      {
        result = new RequestSliverStart(index, nodes, cm.getUrl(index));
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
      clip.text.visible = true;
      clip.textScroll.visible = true;
      clip.rspecText.visible = false;
      clip.rspecScroll.visible = false;
    }

    function clickRspec(event : MouseEvent) : void
    {
      clip.text.visible = ! clip.text.visible;
      clip.textScroll.visible = ! clip.textScroll.visible;
      clip.rspecText.visible = ! clip.rspecText.visible;
      clip.rspecScroll.visible = ! clip.rspecScroll.visible;

      clip.rspecText.text = "";
      var i : int = 0;
      for (; i < ComponentManager.cmNames.length; ++i)
      {
        var xml = nodes.getXml(i, true);
        if (xml != null)
        {
          clip.rspecText.appendText("\n\n\n\n--------------------------------\n");
          clip.rspecText.appendText(ComponentManager.cmNames[i] + "\n");
          clip.rspecText.appendText("--------------------------------\n\n");
          clip.rspecText.appendText(xml.toString());
        }
      }
      clip.rspecScroll.update();
    }

    function start() : void
    {
      if (! queue.isEmpty())
      {
        var front = queue.front();
        var op = front.start(credential);
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

    var clip : ConsoleClip;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var arena : SliceDetailClip;

    var buttons : ButtonList;

    var queue : Queue;
    var working : Boolean;

    var credential : Credential;
  }
}
