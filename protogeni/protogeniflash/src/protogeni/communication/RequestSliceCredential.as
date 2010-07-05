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
	import flash.events.ErrorEvent;

  public class RequestSliceCredential extends Request
  {
    public function RequestSliceCredential() : void
    {
      super("Clearinghouse");
	  name = "GetCredential";
	  op.reset(CommunicationUtil.getCredential);
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
		var result : Request = null;

		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			Main.protogeniHandler.CurrentUser.credential = String(response.value);
			var cred:XML = new XML(response.value);
			Main.protogeniHandler.CurrentUser.urn = cred.credential.owner_urn;
			startSshLookup();
		}
		else
		{
			codeFailure();
		}
		
      if (code == 0)
      {
        var headDiscovery : RequestDiscoverResources = null;
		var tailDiscovery : RequestDiscoverResources = null;
        var headSliver : RequestSliverGet = null;
		var cm:ComponentManager;
		
		// Create the CMs
		for each (var input in response.value)
        {
          var current : ComponentManager
            = new ComponentManager(input.urn, input.hrn, "REMOVEME",
                                   input.url, null, 2);
          view.addManager(current);
		  headDiscovery = new RequestDiscoverResources(current, headDiscovery);
		  if(tailDiscovery == null)
		  	tailDiscovery = headDiscovery;
		  if(credential.existing)
		  	headSliver = new RequestSliverGet(current, (Main.menu as MenuSliceDetail).sliceUrn, headSliver);
        }
		tailDiscovery.next = headSliver;
        result = headDiscovery;
      }
      else
      {
        Main.getConsole().appendText("ListComponents Failed\n");
      }
      return result;
    }
	
	override public function fail(event:ErrorEvent):void
	{
		main.openConsole();
		main.setProgress("Operation failed!", Common.failColor);
		main.stopWaiting();
		outputFailure(event, fault);
		
		var msg:String = event.toString();
		if(msg.search("#2048") > -1)
			Alert.show("Check to make sure you have added a security exception for: " + op.getUrl(), "Security Exception");
		else if(msg.search("#2032") > -1)
			Alert.show("Check to make sure your SSL certificate has been added to your browser.","SSL Certificate");
	}

    private var view : ComponentView;
  }
}
