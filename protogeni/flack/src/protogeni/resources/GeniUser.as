/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
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

package protogeni.resources
{
	import com.mattism.http.xmlrpc.JSLoader;
	
	// ProtoGENI user information
	public class GeniUser
	{
		[Bindable]
		public var uid:String;
		
		[Bindable]
		public var hrn:String;
		[Bindable]
		public var email:String;
		[Bindable]
		public var name:String;
		[Bindable]
		public var authority:SliceAuthority;
		public var credential:String;
		public var keys:Array;
		[Bindable]
		public var urn:IdnUrn;
		
		public var slices:SliceCollection;
		
		public var hasSetupJavascript:Boolean = false;
		
		public function GeniUser()
		{
			slices = new SliceCollection();
		}
		
		// Just some simple encryption so it doesn't store in plain text...
		public function setPassword(password:String,
									store:Boolean):Boolean {
			if(store) {
				if(FlackCache.instance.userPassword != password) {
					FlackCache.instance.userPassword = password;
					FlackCache.instance.save();
				}
			} else if(FlackCache.instance.userPassword.length > 0) {
				FlackCache.instance.userPassword = "";
				FlackCache.instance.save();
			}
			return tryToSetInJavascript(password);
		}
		
		public function tryToSetInJavascript(password:String):Boolean {
			if(Main.useJavascript) {
				if(password.length > 0 && FlackCache.instance.userSslPem.length > 0) {
					try {
						JSLoader.setClientInfo(password, FlackCache.instance.userSslPem);
						hasSetupJavascript = true;
					} catch ( e:Error) {
						LogHandler.appendMessage(new LogMessage("JS", "JS User Cert", e.toString(), true, LogMessage.TYPE_END));
						return false;
					}
				} else
					return false;
			}
			return true;
		}
	}
}