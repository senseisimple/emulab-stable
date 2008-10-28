package
{
  import flash.display.SimpleButton;
  import flash.text.TextField;
  import flash.events.Event;
  import flash.events.ErrorEvent;
  import flash.events.MouseEvent;
  import Error;
  import com.mattism.http.xmlrpc.Connection;
  import com.mattism.http.xmlrpc.ConnectionImpl;
  import com.mattism.http.xmlrpc.MethodFault;

  public class Geni
  {
/*
    public static function init(newConsole : TextField,
                                newButton : SimpleButton) : void
    {
      op = new Operation("", "");
      console = newConsole;
      button = newButton;
      button.addEventListener(MouseEvent.CLICK, methodStart);
      credential = null;
      leebee = null;
      sliceUuid = null;
      current = 0;
    }

    static function methodStart(event : MouseEvent) : void
    {
      console.appendText("START: " + name[current] + "\n");
      start[current]();
      button.removeEventListener(MouseEvent.CLICK, methodStart);
      button.visible = false;
    }

    static function methodComplete(code : Number, value : String,
                                   output : String,
                                   response : Object) : void
    {
      if (code == 0)
      {
        console.appendText("COMPLETE: " + name[current] + "\n");
        if (output != "")
        {
          console.appendText("Output: " + output + "\n");
        }
        complete[current](value, response);
        button.addEventListener(MouseEvent.CLICK, methodStart);
        button.visible = true;
      }
      else
      {
        console.appendText("COMPLETE CODE: " + name[current] + ": " + code
                           + "\n");
      }
      ++current;
      if (current >= name.length)
      {
        button.visible = false;
        button.removeEventListener(MouseEvent.CLICK, methodStart);
      }
    }

    static function methodFailure(event : ErrorEvent, fault : MethodFault) : void
    {
      if (fault != null)
      {
        console.appendText("FAILURE: " + name[current] + ": "
                           + fault.getFaultString() + "\n");
      }
      else
      {
        console.appendText("FAILURE: " + name[current] + ": Fault is null: "
                           + event.toString() + "\n");
      }
    }

    static function startCredential() : void
    {
      op.reset("sa", "GetCredential");
      op.addField("uuid", "0b2eb97e-ed30-11db-96cb-001143e453fe");
      op.call(methodComplete, methodFailure);
    }

    static function completeCredential(value : String,
                                       response : Object) : void
    {
      credential = value;
    }

    static function startLeigh() : void
    {
      op.reset("sa", "Resolve");
      op.addField("hrn", "stoller");
      op.addField("credential", credential);
      op.addField("type", "User");
      op.call(methodComplete, methodFailure);
    }

    static function completeNoop(value : String, response : Object) : void
    {
    }

    static function startLeebee() : void
    {
      op.reset("sa", "Resolve");
      op.addField("hrn", "leebee");
      op.addField("credential", credential);
      op.addField("type", "User");
      op.call(methodComplete, methodFailure);
    }

    static function completeLeebee(value : String, response : Object) : void
    {
      leebee = value;
    }

    static function startLookupNode() : void
    {
      op.reset("cm", "Resolve");
      op.addField("hrn", "pc41");
      op.addField("credential", credential);
      op.addField("type", "Node");
      op.setUrl("https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc");
      op.call(methodComplete, methodFailure);
    }

    static function startSliceCheck() : void
    {
      op.reset("sa", "Resolve");
      op.addField("hrn", "myslice1");
      op.addField("credential", credential);
      op.addField("type", "Slice");
      op.call(sliceCheckComplete, methodFailure);
    }

    static function sliceCheckComplete(code : Number, value : String,
                                       output : String,
                                       response : Object) : void
    {
      if (code == 0)
      {
        console.appendText("COMPLETE: Slice exists. We must delete it.\n");
        if (output != "")
        {
          console.appendText("Output: " + output + "\n");
        }
        var slice : Object = response["value"];
        sliceUuid = slice["uuid"];
      }
      else
      {
        console.appendText("COMPLETE: Slice does not exist. "
                           + "Skipping deletion.\n");
        ++current;
      }
      button.addEventListener(MouseEvent.CLICK, methodStart);
      button.visible = true;
      ++current;
      if (current >= name.length)
      {
        button.visible = false;
        button.removeEventListener(MouseEvent.CLICK, methodStart);
      }
    }

    public function startSliceDelete() : void
    {
      op.reset("sa", "Remove");
      op.addField("credential", credential);
      op.addField("uuid",
      op.addField("type", "Slice");
      op.call(sliceCheckComplete, methodFailure);
    }

    static var op : Operation;
    static var console : TextField = null;
    static var button : SimpleButton = null;
    static var credential : String;
    static var leebee : String;
    static var sliceUuid : String;
    static var current : int = 0;

    static var name : Array = new Array("Request Credential",
                                        "Lookup Leigh",
                                        "Lookup Leebee",
                                        "Lookup pc41",
                                        "Slice Check",
                                        "Slice Delete");

    static var start : Array = new Array(startCredential,
                                         startLeigh,
                                         startLeebee,
                                         startLookupNode,
                                         startSliceCheck,
                                         startSliceDelete);

    static var complete : Array = new Array(completeCredential,
                                            completeNoop,
                                            completeLeebee,
                                            completeNoop,
                                            completeNoop,
                                            completeNoop);
  }
*/
}
