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

package protogeni.communication
{
  import com.mattism.http.xmlrpc.MethodFault;
  
  import flash.events.ErrorEvent;

  public class Request
  {
    public function Request(newName:String = "", newDetails:String = "", qualifiedMethod : Array = null, shouldContinueOnErrors:Boolean = false, shouldWaitOnComplete:Boolean = false) : void
    {
      op = new Operation(qualifiedMethod);
      name = newName;
	  details = newDetails;
	  continueOnError = shouldContinueOnErrors;
	  waitOnComplete = shouldWaitOnComplete;
    }

    public function cleanup() : void
    {
      op.cleanup();
    }
	
	public function cancel():void
	{
		cleanup();
	}

    public function getSendXml() : String
    {
      return op.getSendXml();
    }

    public function getResponseXml() : String
    {
      return op.getResponseXml();
    }

    public function getUrl() : String
    {
      return op.getUrl();
    }

    public function start() : Operation
    {
      return op;
    }

    public function complete(code : Number, response : Object) : *
    {
      return null;
    }

    public function fail(event : ErrorEvent, fault : MethodFault) : *
    {
		return null;
    }

	public var op:Operation;
	[Bindable]
	public var name:String;
	[Bindable]
	public var details:String;
	public var waitOnComplete:Boolean;
	public var continueOnError:Boolean;
  }
}
