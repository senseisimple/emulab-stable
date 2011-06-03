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
	public class LoginService
	{
		public var authentication:String;
		public var hostname:String;
		public var port:String;
		public var username:String;
		
		public function LoginService(newAuth:String = "",
									 newHost:String = "",
									 newPort:String = "",
									 newUser:String = "")
		{
			this.authentication = newAuth;
			this.hostname = newHost;
			this.port = newPort;
			this.username = newUser;
		}
	}
}