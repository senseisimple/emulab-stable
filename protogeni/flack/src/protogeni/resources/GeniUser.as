/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
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
	import com.hurlant.crypto.symmetric.AESKey;
	import com.hurlant.util.Hex;
	import com.mattism.http.xmlrpc.JSLoader;
	
	import flash.utils.ByteArray;
	
	// ProtoGENI user information
	public class GeniUser
	{
		[Bindable]
		public var uid:String;
		
		[Bindable]
		public var uuid:String;
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
		public var urn:String;
		
		public var slices:SliceCollection;
		
		public var encryptedPassword:String = "";
		[Bindable]
		public var sslPem:String = "";
		
		public var hasSetupJavascript:Boolean = false;
		
		public function GeniUser()
		{
			slices = new SliceCollection();
			sslPem = "";
		}
		
		// Just some simple encryption so it doesn't store in plain text...
		public function setPassword(password:String, store:Boolean):Boolean {
			/*
			var key:ByteArray = Hex.toArray("00010203050607080A0B0C0D0F101112");
			var pt:ByteArray = Hex.toArray(password);
			var aes:AESKey = new AESKey(key);
			aes.encrypt(pt);
			encryptedPassword = Hex.fromArray(pt);
			*/
			if(store)
				encryptedPassword = password;
			return tryToSetInJavascript(password);
		}
		
		public function getPassword():String {
			if(encryptedPassword == "")
				return "";
			else
				return this.encryptedPassword;
			/*
			var key:ByteArray = Hex.toArray("00010203050607080A0B0C0D0F101112");
			var pt:ByteArray = Hex.toArray(encryptedPassword);
			var aes:AESKey = new AESKey(key);
			aes.decrypt(pt);
			return Hex.fromArray(pt);
			*/
		}
		
		public function tryToSetInJavascript(password:String):Boolean {
			if(Main.useJavascript) {
				if(password.length > 0 && sslPem.length > 0) {
					try {
						JSLoader.setClientInfo(password, sslPem);
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