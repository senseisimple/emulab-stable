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
  import flash.events.ErrorEvent;
  import flash.events.Event;
  import com.mattism.http.xmlrpc.MethodFault;

  class MenuSliceCreate extends MenuState
  {
    public function MenuSliceCreate(newSliceName : String) : void
    {
      sliceName = newSliceName;
      clip = null;
      sliceId = null;
      op = null;
      opName = null;
      credential = null;
      user = null;
      credential = new Credential();
    }

    override public function init(newParent : DisplayObjectContainer) : void
    {
      clip = new WaitingClip();
      newParent.addChild(clip);

      clip.addEventListener(Event.ENTER_FRAME, enterFrame);

      op = new Operation(null);
      startCredential();
    }

    override public function cleanup() : void
    {
      op.cleanup();
      clip.removeEventListener(Event.ENTER_FRAME, enterFrame);
      clip.parent.removeChild(clip);
    }

    override public function getConsole() : TextField
    {
      return clip.xmlText;
    }

    function enterFrame(event : Event) : void
    {
      if (clip.xmlText.scrollV < clip.xmlText.maxScrollV)
      {
        clip.xmlText.scrollV += 3;
      }
      clip.waitIcon.rotation += 5;
    }

    function failure(event : ErrorEvent, fault : MethodFault) : void
    {
      clip.removeEventListener(Event.ENTER_FRAME, enterFrame);
      if (fault != null)
      {
        clip.xmlText.appendText("\nFAILURE fault: " + opName + ": "
                                + fault.getFaultString());
      }
      else
      {
        clip.xmlText.appendText("\nFAILURE event: " + opName + ": "
                                + event.toString());
      }
      clip.xmlText.appendText("\nURL: " + op.getUrl());
    }

    function codeFailure() : void
    {
      clip.removeEventListener(Event.ENTER_FRAME, enterFrame);
      clip.loadText.text = "Code Failure: " + opName;
    }

    function addSend() : void
    {
      var before : int = clip.xmlText.scrollV;
      clip.xmlText.appendText("\n-----------------------------------------\n");
      clip.xmlText.appendText("SEND: " + opName + "\n");
      clip.xmlText.appendText("URL: " + op.getUrl() + "\n");
      clip.xmlText.appendText("-----------------------------------------\n\n");
      clip.xmlText.appendText(op.getSendXml());
      clip.xmlText.scrollV = before;
//      clip.xmlText.scrollV = clip.xmlText.maxScrollV;
    }

    function addResponse() : void
    {
      var before : int = clip.xmlText.scrollV;
      clip.xmlText.appendText("\n-----------------------------------------\n");
      clip.xmlText.appendText("RESPONSE: " + opName + "\n");
      clip.xmlText.appendText("-----------------------------------------\n\n");
      clip.xmlText.appendText(op.getResponseXml());
      clip.xmlText.scrollV = before;
    }

    function startCredential() : void
    {
      opName = "Acquiring credential";
      clip.loadText.text = opName;
      op.reset(Geni.getCredential);
//      op.addField("uuid", "0b2eb97e-ed30-11db-96cb-001143e453fe");
      op.call(completeCredential, failure);
      addSend();
    }

    function completeCredential(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        credential.base = String(response.value);
        startSshLookup();
      }
      else
      {
        codeFailure();
      }
    }

    function startSshLookup() : void
    {
      opName = "Acquiring SSH Keys";
      clip.loadText.text = opName;
      op.reset(Geni.getKeys);
      op.addField("credential", credential.base);
      op.call(completeSshLookup, failure);
      addSend();
    }

    function completeSshLookup(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        credential.ssh = response.value;
        startSliceDelete();
//        startUserLookup();
      }
      else
      {
        codeFailure();
      }
    }


    function startUserLookup() : void
    {
      opName = "Lookup User";
      clip.loadText.text = "Looking up user '" + userName + "'";
      op.reset(Geni.resolve);
      op.addField("hrn", userName);
      op.addField("credential", credential.base);
      op.addField("type", "User");
      op.call(completeUserLookup, failure);
      addSend();
    }

    function completeUserLookup(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        user = response.value;
        startSliceDelete();
      }
      else
      {
        codeFailure();
      }
    }

    function startSliceLookup() : void
    {
      opName = "Lookup Slice";
      clip.loadText.text = "Looking up slice name '" + sliceName + "'";
      op.reset(Geni.resolve);
      op.addField("hrn", sliceName);
      op.addField("credential", credential.base);
      op.addField("type", "Slice");
      op.call(completeSliceLookup, failure);
      addSend();
    }

    function completeSliceLookup(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        sliceId = response.value.uuid;
        startSliceDelete();
      }
      else
      {
        startSliceCreate();
      }
    }

    function startSliceDelete() : void
    {
      opName = "Delete";
      clip.loadText.text = "Deleting existing slice";
      op.reset(Geni.remove);
      op.addField("credential", credential.base);
      op.addField("hrn", Util.makeUrn(Geni.defaultAuthority, "slice",
                                      sliceName));
      op.addField("type", "Slice");
      op.call(completeSliceDelete, failure);
      addSend();
    }

    function completeSliceDelete(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0 || code == 12)
      {
        startSliceCreate();
      }
          else if(code == 7)
          {
                  var newMenu = new MenuSliceSelect();
                  Main.changeState(newMenu);
                  newMenu.setSliceName(sliceName + " hasn't expired");
          }
      else
      {
        codeFailure();
      }
    }

    function startSliceCreate() : void
    {
      opName = "Create";
      clip.loadText.text = "Creating new slice";
      op.reset(Geni.register);
      op.addField("credential", credential.base);
      op.addField("hrn", Util.makeUrn(Geni.defaultAuthority, "slice",
                                      sliceName));
//      op.addField("hrn", sliceName);
      op.addField("type", "Slice");
//      op.addField("userbindings", new Array(user.uuid));
      op.call(completeSliceCreate, failure);
      addSend();
    }

    function completeSliceCreate(code : Number, response : Object) : void
    {
      addResponse();
      if (code == 0)
      {
        credential.slice = String(response.value);
//        startResourceLookup();
        var newMenu = new MenuSliceDetail(sliceName, sliceId, credential);
        Main.setText(clip.xmlText.text);
        Main.changeState(newMenu);
      }
      else
      {
        codeFailure();
      }
    }

    function startResourceLookup() : void
    {
      var rspec : String =
        "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> " +
        " <node uuid=\"00000000-0000-0000-0000-000000000002\" " +
        "       virtualization_type=\"raw\"> " +
        " </node>" +
        "</rspec>";
      opName = "Looking up resources";
      clip.loadText.text = opName;
      op.reset(Geni.discoverResources);
      op.addField("credentials", new Array(credential.slice));
      op.addField("rspec", rspec);
      op.setUrl("https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc");
      op.call(completeResourceLookup, failure);
    }

    function completeResourceLookup(code : Number, response : Object) : void
    {
      if (code == 0)
      {
        clip.xmlText.text = String(response.value);
      }
      else
      {
        codeFailure();
        clip.xmlText.text = op.getResponseXml();
      }
    }

    var clip : WaitingClip;
    var sliceName : String;
    var sliceId : String;
    var op : Operation;
    var opName : String;
    var user : Object;
    var credential : Credential;

    static var userName : String = "duerig";
  }
}
